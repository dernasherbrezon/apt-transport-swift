#include "HttpServerState.h";
#include <sstream>

// UnwrapHTTPConnect - Does the HTTP CONNECT handshake			/*{{{*/
// ---------------------------------------------------------------------
/* Performs a TLS handshake on the socket */
struct HttpConnectFd : public MethodFd
{
   std::unique_ptr<MethodFd> UnderlyingFd;
   std::string Buffer;

   int Fd() APT_OVERRIDE { return UnderlyingFd->Fd(); }

   ssize_t Read(void *buf, size_t count) APT_OVERRIDE
   {
      if (!Buffer.empty())
      {
	 auto read = count < Buffer.size() ? count : Buffer.size();

	 memcpy(buf, Buffer.data(), read);
	 Buffer.erase(Buffer.begin(), Buffer.begin() + read);
	 return read;
      }

      return UnderlyingFd->Read(buf, count);
   }
   ssize_t Write(void *buf, size_t count) APT_OVERRIDE
   {
      return UnderlyingFd->Write(buf, count);
   }

   int Close() APT_OVERRIDE
   {
      return UnderlyingFd->Close();
   }

   bool HasPending() APT_OVERRIDE
   {
      return !Buffer.empty();
   }
};


static ResultState UnwrapHTTPConnect(std::string Host, int Port, URI Proxy, std::unique_ptr<MethodFd> &Fd,
				     unsigned long Timeout, aptAuthConfMethod *Owner)
{
   Owner->Status(_("Connecting to %s (%s)"), "HTTP proxy", URI::SiteOnly(Proxy).c_str());
   // The HTTP server expects a hostname with a trailing :port
   std::stringstream Req;
   std::string ProperHost;

   if (Host.find(':') != std::string::npos)
      ProperHost = '[' + Proxy.Host + ']';
   else
      ProperHost = Proxy.Host;

   // Build the connect
   Req << "CONNECT " << Host << ":" << std::to_string(Port) << " HTTP/1.1\r\n";
   if (Proxy.Port != 0)
      Req << "Host: " << ProperHost << ":" << std::to_string(Proxy.Port) << "\r\n";
   else
      Req << "Host: " << ProperHost << "\r\n";

   Owner->MaybeAddAuthTo(Proxy);
   if (Proxy.User.empty() == false || Proxy.Password.empty() == false)
      Req << "Proxy-Authorization: Basic "
	  << Base64Encode(Proxy.User + ":" + Proxy.Password) << "\r\n";

   Req << "User-Agent: " << Owner->ConfigFind("User-Agent", "Debian APT-HTTP/1.3 (" PACKAGE_VERSION ")") << "\r\n";

   Req << "\r\n";

   CircleBuf In(Owner->ConfigFindI("Dl-Limit", 0), 4096);
   CircleBuf Out(Owner->ConfigFindI("Dl-Limit", 0), 4096);
   std::string Headers;

   if (Owner->DebugEnabled() == true)
      std::cerr << Req.str() << std::endl;
   Out.Read(Req.str());

   // Writing from proxy
   while (Out.WriteSpace() > 0)
   {
      if (WaitFd(Fd->Fd(), true, Timeout) == false)
      {
	 _error->Errno("select", "Writing to proxy failed");
	 return ResultState::TRANSIENT_ERROR;
      }
      if (Out.Write(Fd) == false)
      {
	 _error->Errno("write", "Writing to proxy failed");
	 return ResultState::TRANSIENT_ERROR;
      }
   }

   while (In.ReadSpace() > 0)
   {
      if (WaitFd(Fd->Fd(), false, Timeout) == false)
      {
	 _error->Errno("select", "Reading from proxy failed");
	 return ResultState::TRANSIENT_ERROR;
      }
      if (In.Read(Fd) == false)
      {
	 _error->Errno("read", "Reading from proxy failed");
	 return ResultState::TRANSIENT_ERROR;
      }

      if (In.WriteTillEl(Headers))
	 break;
   }

   if (Owner->DebugEnabled() == true)
	   std::cerr << Headers << std::endl;

   if (!(APT::String::Startswith(Headers, "HTTP/1.0 200") || APT::String::Startswith(Headers, "HTTP/1.1 200")))
   {
      _error->Error("Invalid response from proxy: %s", Headers.c_str());
      return ResultState::TRANSIENT_ERROR;
   }

   if (In.WriteSpace() > 0)
   {
      // Maybe there is actual data already read, if so we need to buffer it
      std::unique_ptr<HttpConnectFd> NewFd(new HttpConnectFd());
      In.Write(NewFd->Buffer);
      NewFd->UnderlyingFd = std::move(Fd);
      Fd = std::move(NewFd);
   }

   return ResultState::SUCCESSFUL;
}
									/*}}}*/

// HttpServerState::HttpServerState - Constructor			/*{{{*/
HttpServerState::HttpServerState(URI Srv,BaseHttpMethod *Owner) : ServerState(Srv, Owner), In(Owner, 64*1024), Out(Owner, 4*1024)
{
   TimeOut = Owner->ConfigFindI("Timeout", TimeOut);
   ServerFd = MethodFd::FromFd(-1);
   Reset();
}
									/*}}}*/
// HttpServerState::Open - Open a connection to the server		/*{{{*/
// ---------------------------------------------------------------------
/* This opens a connection to the server. */
ResultState HttpServerState::Open()
{
   // Use the already open connection if possible.
   if (ServerFd->Fd() != -1)
      return ResultState::SUCCESSFUL;

   Close();
   In.Reset();
   Out.Reset();
   Persistent = true;

   // Determine the proxy setting
   // Used to run AutoDetectProxy(ServerName) here, but we now send a Proxy
   // header in the URI Acquire request and set "Acquire::"+uri.Access+"::proxy::"+uri.Host
   // to it in BaseSwiftMethod::Loop()
   string SpecificProxy = Owner->ConfigFind("Proxy::" + ServerName.Host, "");
   if (!SpecificProxy.empty())
   {
	   if (SpecificProxy == "DIRECT")
		   Proxy = "";
	   else
		   Proxy = SpecificProxy;
   }
   else
   {
	   string DefProxy = Owner->ConfigFind("Proxy", "");
	   if (!DefProxy.empty())
	   {
		   Proxy = DefProxy;
	   }
	   else
	   {
		 char *result = getenv("https_proxy");
		 if (result != nullptr)
		 {
		    Proxy = result;
		 } else {
			 Proxy = "";
		 }
	   }
   }

   // Parse no_proxy, a , separated list of domains
   if (getenv("no_proxy") != 0)
   {
      if (CheckDomainList(ServerName.Host,getenv("no_proxy")) == true)
	 Proxy = "";
   }

   if (Proxy.empty() == false)
      Owner->AddProxyAuth(Proxy, ServerName);

   auto const DefaultService = "https";
   auto const DefaultPort = 443;
   if (Proxy.Access == "socks5h")
   {
      auto result = Connect(Proxy.Host, Proxy.Port, "socks", 1080, ServerFd, TimeOut, Owner);
      if (result != ResultState::SUCCESSFUL)
	 return result;

      result = UnwrapSocks(ServerName.Host, ServerName.Port == 0 ? DefaultPort : ServerName.Port,
			   Proxy, ServerFd, Owner->ConfigFindI("TimeOut", 120), Owner);
      if (result != ResultState::SUCCESSFUL)
	 return result;
   }
   else
   {
      // Determine what host and port to use based on the proxy settings
      int Port = 0;
      string Host;
      if (Proxy.empty() == true || Proxy.Host.empty() == true)
      {
	 if (ServerName.Port != 0)
	    Port = ServerName.Port;
	 Host = ServerName.Host;
      }
      else if (Proxy.Access != "http" && Proxy.Access != "https")
      {
	 _error->Error("Unsupported proxy configured: %s", URI::SiteOnly(Proxy).c_str());
	 return ResultState::FATAL_ERROR;
      }
      else
      {
	 if (Proxy.Port != 0)
	    Port = Proxy.Port;
	 Host = Proxy.Host;

	 if (Proxy.Access == "https" && Port == 0)
	    Port = 443;
      }
      auto result = Connect(Host, Port, DefaultService, DefaultPort, ServerFd, TimeOut, Owner);
      if (result != ResultState::SUCCESSFUL)
	 return result;
      if (Host == Proxy.Host && Proxy.Access == "https")
      {
	 result = UnwrapTLS(Proxy.Host, ServerFd, TimeOut, Owner);
	 if (result != ResultState::SUCCESSFUL)
	    return result;
      }
      if (Host == Proxy.Host)
      {
	 result = UnwrapHTTPConnect(ServerName.Host, ServerName.Port == 0 ? DefaultPort : ServerName.Port, Proxy, ServerFd, Owner->ConfigFindI("TimeOut", 120), Owner);
	 if (result != ResultState::SUCCESSFUL)
	    return result;
      }
   }

   return UnwrapTLS(ServerName.Host, ServerFd, TimeOut, Owner);

}

									/*}}}*/
// HttpServerState::Close - Close a connection to the server		/*{{{*/
// ---------------------------------------------------------------------
/* */
bool HttpServerState::Close()
{
   ServerFd->Close();
   return true;
}
									/*}}}*/
// HttpServerState::RunData - Transfer the data from the socket		/*{{{*/
ResultState HttpServerState::RunData(RequestState &Req)
{
   Req.State = RequestState::Data;

   // Chunked transfer encoding is fun..
   if (Req.Encoding == RequestState::Chunked)
   {
      while (1)
      {
	 // Grab the block size
	 ResultState Last = ResultState::SUCCESSFUL;
	 string Data;
	 In.Limit(-1);
	 do
	 {
	    if (In.WriteTillEl(Data,true) == true)
	       break;
	 } while ((Last = Go(false, Req)) == ResultState::SUCCESSFUL);

	 if (Last != ResultState::SUCCESSFUL)
	    return Last;

	 // See if we are done
	 unsigned long long Len = strtoull(Data.c_str(),0,16);
	 if (Len == 0)
	 {
	    In.Limit(-1);

	    // We have to remove the entity trailer
	    Last = ResultState::SUCCESSFUL;
	    do
	    {
	       if (In.WriteTillEl(Data,true) == true && Data.length() <= 2)
		  break;
	    } while ((Last = Go(false, Req)) == ResultState::SUCCESSFUL);
	    return Last;
	 }

	 // Transfer the block
	 In.Limit(Len);
	 while (Go(true, Req) == ResultState::SUCCESSFUL)
	    if (In.IsLimit() == true)
	       break;

	 // Error
	 if (In.IsLimit() == false)
	    return ResultState::TRANSIENT_ERROR;

	 // The server sends an extra new line before the next block specifier..
	 In.Limit(-1);
	 Last = ResultState::SUCCESSFUL;
	 do
	 {
	    if (In.WriteTillEl(Data,true) == true)
	       break;
	 } while ((Last = Go(false, Req)) == ResultState::SUCCESSFUL);
	 if (Last != ResultState::SUCCESSFUL)
	    return Last;
      }
   }
   else
   {
      /* Closes encoding is used when the server did not specify a size, the
         loss of the connection means we are done */
      if (Req.JunkSize != 0)
	 In.Limit(Req.JunkSize);
      else if (Req.DownloadSize != 0)
      {
	 if (Req.MaximumSize != 0 && Req.DownloadSize > Req.MaximumSize)
	 {
	    Owner->SetFailReason("MaximumSizeExceeded");
	    _error->Error(_("File has unexpected size (%llu != %llu). Mirror sync in progress?"),
			  Req.DownloadSize, Req.MaximumSize);
	    return ResultState::FATAL_ERROR;
	 }
	 In.Limit(Req.DownloadSize);
      }
      else if (Persistent == false)
	 In.Limit(-1);

      // Just transfer the whole block.
      while (true)
      {
	 if (In.IsLimit() == false)
	 {
	    auto const result = Go(true, Req);
	    if (result == ResultState::SUCCESSFUL)
	       continue;
	    return result;
	 }

	 In.Limit(-1);
	 return _error->PendingError() ? ResultState::FATAL_ERROR : ResultState::SUCCESSFUL;
      }
   }

   if (Flush(&Req.File) == false)
      return ResultState::TRANSIENT_ERROR;
   return ResultState::SUCCESSFUL;
}
									/*}}}*/
ResultState HttpServerState::RunDataToDevNull(RequestState &Req) /*{{{*/
{
   // no need to clean up if we discard the connection anyhow
   if (Persistent == false)
      return ResultState::SUCCESSFUL;
   Req.File.Open("/dev/null", FileFd::WriteOnly);
   return RunData(Req);
}
									/*}}}*/
bool HttpServerState::ReadHeaderLines(std::string &Data)		/*{{{*/
{
   return In.WriteTillEl(Data);
}
									/*}}}*/
ResultState HttpServerState::LoadNextResponse(bool const ToFile, RequestState &Req) /*{{{*/
{
   return Go(ToFile, Req);
}
									/*}}}*/
bool HttpServerState::WriteResponse(const std::string &Data)		/*{{{*/
{
   return Out.Read(Data);
}
									/*}}}*/
APT_PURE bool HttpServerState::IsOpen()					/*{{{*/
{
   return (ServerFd->Fd() != -1);
}
									/*}}}*/
bool HttpServerState::InitHashes(HashStringList const &ExpectedHashes)	/*{{{*/
{
   delete In.Hash;
   In.Hash = new Hashes(ExpectedHashes);
   return true;
}
									/*}}}*/
void HttpServerState::Reset()						/*{{{*/
{
   ServerState::Reset();
   ServerFd->Close();
}
									/*}}}*/

APT_PURE Hashes * HttpServerState::GetHashes()				/*{{{*/
{
   return In.Hash;
}
									/*}}}*/
// HttpServerState::Die - The server has closed the connection.		/*{{{*/
ResultState HttpServerState::Die(RequestState &Req)
{
   unsigned int LErrno = errno;

   // Dump the buffer to the file
   if (Req.State == RequestState::Data)
   {
      if (Req.File.IsOpen() == false)
	 return ResultState::SUCCESSFUL;
      // on GNU/kFreeBSD, apt dies on /dev/null because non-blocking
      // can't be set
      if (Req.File.Name() != "/dev/null")
	 SetNonBlock(Req.File.Fd(),false);
      while (In.WriteSpace() == true)
      {
	 if (In.Write(MethodFd::FromFd(Req.File.Fd())) == false)
	 {
	    _error->Errno("write", _("Error writing to the file"));
	    return ResultState::TRANSIENT_ERROR;
	 }

	 // Done
	 if (In.IsLimit() == true)
	    return ResultState::SUCCESSFUL;
      }
   }

   // See if this is because the server finished the data stream
   if (In.IsLimit() == false && Req.State != RequestState::Header &&
       Persistent == true)
   {
      Close();
      if (LErrno == 0)
      {
	 _error->Error(_("Error reading from server. Remote end closed connection"));
	 return ResultState::TRANSIENT_ERROR;
      }
      errno = LErrno;
      _error->Errno("read", _("Error reading from server"));
      return ResultState::TRANSIENT_ERROR;
   }
   else
   {
      In.Limit(-1);

      // Nothing left in the buffer
      if (In.WriteSpace() == false)
	 return ResultState::TRANSIENT_ERROR;

      // We may have got multiple responses back in one packet..
      Close();
      return ResultState::SUCCESSFUL;
   }

   return ResultState::TRANSIENT_ERROR;
}
									/*}}}*/
// HttpServerState::Flush - Dump the buffer into the file		/*{{{*/
// ---------------------------------------------------------------------
/* This takes the current input buffer from the Server FD and writes it
   into the file */
bool HttpServerState::Flush(FileFd * const File)
{
   if (File != nullptr)
   {
      // on GNU/kFreeBSD, apt dies on /dev/null because non-blocking
      // can't be set
      if (File->Name() != "/dev/null")
	 SetNonBlock(File->Fd(),false);
      if (In.WriteSpace() == false)
	 return true;

      while (In.WriteSpace() == true)
      {
	 if (In.Write(MethodFd::FromFd(File->Fd())) == false)
	    return _error->Errno("write",_("Error writing to file"));
	 if (In.IsLimit() == true)
	    return true;
      }

      if (In.IsLimit() == true || Persistent == false)
	 return true;
   }
   return false;
}
									/*}}}*/
// HttpServerState::Go - Run a single loop				/*{{{*/
// ---------------------------------------------------------------------
/* This runs the select loop over the server FDs, Output file FDs and
   stdin. */
ResultState HttpServerState::Go(bool ToFile, RequestState &Req)
{
   // Server has closed the connection
   if (ServerFd->Fd() == -1 && (In.WriteSpace() == false ||
				ToFile == false))
      return ResultState::TRANSIENT_ERROR;

   // Handle server IO
   if (ServerFd->HasPending() && In.ReadSpace() == true)
   {
      errno = 0;
      if (In.Read(ServerFd) == false)
	 return Die(Req);
   }

   fd_set rfds,wfds;
   FD_ZERO(&rfds);
   FD_ZERO(&wfds);

   /* Add the server. We only send more requests if the connection will
      be persisting */
   if (Out.WriteSpace() == true && ServerFd->Fd() != -1 && Persistent == true)
      FD_SET(ServerFd->Fd(), &wfds);
   if (In.ReadSpace() == true && ServerFd->Fd() != -1)
      FD_SET(ServerFd->Fd(), &rfds);

   // Add the file
   auto FileFD = MethodFd::FromFd(-1);
   if (Req.File.IsOpen())
      FileFD = MethodFd::FromFd(Req.File.Fd());

   if (In.WriteSpace() == true && ToFile == true && FileFD->Fd() != -1)
      FD_SET(FileFD->Fd(), &wfds);

   // Add stdin
   if (Owner->ConfigFindB("DependOnSTDIN", true) == true)
      FD_SET(STDIN_FILENO,&rfds);

   // Figure out the max fd
   int MaxFd = FileFD->Fd();
   if (MaxFd < ServerFd->Fd())
      MaxFd = ServerFd->Fd();

   // Select
   struct timeval tv;
   tv.tv_sec = TimeOut;
   tv.tv_usec = 0;
   int Res = 0;
   if ((Res = select(MaxFd+1,&rfds,&wfds,0,&tv)) < 0)
   {
      if (errno == EINTR)
	 return ResultState::SUCCESSFUL;
      _error->Errno("select", _("Select failed"));
      return ResultState::TRANSIENT_ERROR;
   }

   if (Res == 0)
   {
      _error->Error(_("Connection timed out"));
      return Die(Req);
   }

   // Handle server IO
   if (ServerFd->Fd() != -1 && FD_ISSET(ServerFd->Fd(), &rfds))
   {
      errno = 0;
      if (In.Read(ServerFd) == false)
	 return Die(Req);
   }

   if (ServerFd->Fd() != -1 && FD_ISSET(ServerFd->Fd(), &wfds))
   {
      errno = 0;
      if (Out.Write(ServerFd) == false)
	 return Die(Req);
   }

   // Send data to the file
   if (FileFD->Fd() != -1 && FD_ISSET(FileFD->Fd(), &wfds))
   {
      if (In.Write(FileFD) == false)
      {
	 _error->Errno("write", _("Error writing to output file"));
	 return ResultState::TRANSIENT_ERROR;
      }
   }

   if (Req.MaximumSize > 0 && Req.File.IsOpen() && Req.File.Failed() == false && Req.File.Tell() > Req.MaximumSize)
   {
      Owner->SetFailReason("MaximumSizeExceeded");
      _error->Error(_("File has unexpected size (%llu != %llu). Mirror sync in progress?"),
		    Req.File.Tell(), Req.MaximumSize);
      return ResultState::FATAL_ERROR;
   }

   // Handle commands from APT
   if (FD_ISSET(STDIN_FILENO,&rfds))
   {
      if (Owner->Run(true) != -1)
	 exit(100);
   }

   return ResultState::SUCCESSFUL;
}
									/*}}}*/
