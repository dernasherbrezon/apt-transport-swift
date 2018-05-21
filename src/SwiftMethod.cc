// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   HTTP Acquire Method - This is the HTTP acquire method for APT.
   
   It uses HTTP/1.1 and many of the fancy options there-in, such as
   pipelining, range, if-range and so on. 

   It is based on a doubly buffered select loop. A groupe of requests are 
   fed into a single output buffer that is constantly fed out the 
   socket. This provides ideal pipelining as in many cases all of the
   requests will fit into a single packet. The input socket is buffered 
   the same way and fed into the fd for the file (may be a pipe in future).
      
   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#include "config.h"

#include <sstream>

#include "connect.h"
#include "HttpServerState.h"
#include "SwiftMethod.h"
#include "apti18n.h"

using namespace std;

unsigned long long CircleBuf::BwReadLimit=0;
unsigned long long CircleBuf::BwTickReadData=0;
struct timeval CircleBuf::BwReadTick={0,0};
const unsigned int CircleBuf::BW_HZ=10;

// SwiftMethod::SendReq - Send the HTTP request				/*{{{*/
// ---------------------------------------------------------------------
/* This places the http request in the outbound buffer */
void SwiftMethod::SendReq(FetchItem *Itm)
{
   URI Uri = Itm->Uri;

   // The HTTP server expects a hostname with a trailing :port
   std::stringstream Req;
   string ProperHost;

   if (Uri.Host.find(':') != string::npos)
      ProperHost = '[' + Uri.Host + ']';
   else
      ProperHost = Uri.Host;

   /* RFC 2616 §5.1.2 requires absolute URIs for requests to proxies,
      but while its a must for all servers to accept absolute URIs,
      it is assumed clients will sent an absolute path for non-proxies */
   std::string requesturi;
   if ((Server->Proxy.Access != "http" && Server->Proxy.Access != "https") || APT::String::Endswith(Uri.Access, "https") || Server->Proxy.empty() == true || Server->Proxy.Host.empty())
      requesturi = Uri.Path;
   else
      requesturi = Uri;

   /* Build the request. No keep-alive is included as it is the default
      in 1.1, can cause problems with proxies, and we are an HTTP/1.1
      client anyway.
      C.f. https://tools.ietf.org/wg/httpbis/trac/ticket/158 */
   Req << "GET " << requesturi << " HTTP/1.1\r\n";
   if (Uri.Port != 0)
      Req << "Host: " << ProperHost << ":" << std::to_string(Uri.Port) << "\r\n";
   else
      Req << "Host: " << ProperHost << "\r\n";

   // generate a cache control header (if needed)
   if (ConfigFindB("No-Cache",false) == true) {
      Req << "Cache-Control: no-cache\r\n" << "Pragma: no-cache\r\n";
   } else if (Itm->IndexFile == true) {
      Req << "Cache-Control: max-age=" << std::to_string(ConfigFindI("Max-Age", 0)) << "\r\n";
   } else if (ConfigFindB("No-Store", false) == true) {
      Req << "Cache-Control: no-store\r\n";
   }

   // If we ask for uncompressed files servers might respond with content-
   // negotiation which lets us end up with compressed files we do not support,
   // see 657029, 657560 and co, so if we have no extension on the request
   // ask for text only. As a sidenote: If there is nothing to negotate servers
   // seem to be nice and ignore it.
   if (ConfigFindB("SendAccept", true) == true)
   {
      size_t const filepos = Itm->Uri.find_last_of('/');
      string const file = Itm->Uri.substr(filepos + 1);
      if (flExtension(file) == file)
	 Req << "Accept: text/*\r\n";
   }

   // Check for a partial file and send if-queries accordingly
   struct stat SBuf;
   if (Server->RangesAllowed && stat(Itm->DestFile.c_str(),&SBuf) >= 0 && SBuf.st_size > 0)
      Req << "Range: bytes=" << std::to_string(SBuf.st_size) << "-\r\n"
	 << "If-Range: " << TimeRFC1123(SBuf.st_mtime, false) << "\r\n";
   else if (Itm->LastModified != 0)
      Req << "If-Modified-Since: " << TimeRFC1123(Itm->LastModified, false).c_str() << "\r\n";

   if ((Server->Proxy.Access == "http" || Server->Proxy.Access == "https") &&
       (Server->Proxy.User.empty() == false || Server->Proxy.Password.empty() == false))
      Req << "Proxy-Authorization: Basic "
	 << Base64Encode(Server->Proxy.User + ":" + Server->Proxy.Password) << "\r\n";

   MaybeAddAuthTo(Uri);
   if (Uri.User.empty() == false || Uri.Password.empty() == false)
      Req << "Authorization: Basic "
	 << Base64Encode(Uri.User + ":" + Uri.Password) << "\r\n";

   Req << "User-Agent: " << ConfigFind("User-Agent",
		"Debian APT-HTTP/1.3 (1.0)") << "\r\n";

   Req << "\r\n";

   if (Debug == true)
	   std::cerr << Req.str() << std::endl;

   Server->WriteResponse(Req.str());
}
									/*}}}*/
std::unique_ptr<ServerState> SwiftMethod::CreateServerState(URI const &uri)/*{{{*/
{
   return std::unique_ptr<ServerState>(new HttpServerState(uri, this));
}
									/*}}}*/
void SwiftMethod::RotateDNS()						/*{{{*/
{
   ::RotateDNS();
}
									/*}}}*/
BaseHttpMethod::DealWithHeadersResult SwiftMethod::DealWithHeaders(FetchResult &Res, RequestState &Req)/*{{{*/
{
   auto ret = BaseHttpMethod::DealWithHeaders(Res, Req);
   if (ret != BaseHttpMethod::FILE_IS_OPEN)
      return ret;
   if (Req.File.Open(Queue->DestFile, FileFd::WriteAny) == false)
      return ERROR_NOT_FROM_SERVER;

   FailFile = Queue->DestFile;
   FailFile.c_str();   // Make sure we don't do a malloc in the signal handler
   FailFd = Req.File.Fd();
   FailTime = Req.Date;

   if (Server->InitHashes(Queue->ExpectedHashes) == false || Req.AddPartialFileToHashes(Req.File) == false)
   {
      _error->Errno("read",_("Problem hashing file"));
      return ERROR_NOT_FROM_SERVER;
   }
   if (Req.StartPos > 0)
      Res.ResumePoint = Req.StartPos;

   SetNonBlock(Req.File.Fd(),true);
   return FILE_IS_OPEN;
}
									/*}}}*/
SwiftMethod::SwiftMethod(std::string &&pProg) : BaseHttpMethod(std::move(pProg), "1.0", Pipeline | SendConfig) /*{{{*/
{
   SeccompFlags = aptMethod::BASE | aptMethod::NETWORK;

   auto addName = std::inserter(methodNames, methodNames.begin());
   if (Binary != "http")
      addName = "http";
   auto const plus = Binary.find('+');
   if (plus != std::string::npos)
   {
      auto name2 = Binary.substr(plus + 1);
      if (std::find(methodNames.begin(), methodNames.end(), name2) == methodNames.end())
	 addName = std::move(name2);
      addName = Binary.substr(0, plus);
   }
}
									/*}}}*/
