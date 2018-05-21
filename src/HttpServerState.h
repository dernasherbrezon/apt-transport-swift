

#ifndef HTTPSERVERSTATE_H_
#define HTTPSERVERSTATE_H_

#include "basehttp.h"
#include "CircleBuf.h"

struct HttpServerState: public ServerState
{
   // This is the connection itself. Output is data FROM the server
   CircleBuf In;
   CircleBuf Out;
   std::unique_ptr<MethodFd> ServerFd;

   protected:
   virtual bool ReadHeaderLines(std::string &Data) APT_OVERRIDE;
   virtual ResultState LoadNextResponse(bool const ToFile, RequestState &Req) APT_OVERRIDE;
   virtual bool WriteResponse(std::string const &Data) APT_OVERRIDE;

   public:
   virtual void Reset() APT_OVERRIDE;

   virtual ResultState RunData(RequestState &Req) APT_OVERRIDE;
   virtual ResultState RunDataToDevNull(RequestState &Req) APT_OVERRIDE;

   virtual ResultState Open() APT_OVERRIDE;
   virtual bool IsOpen() APT_OVERRIDE;
   virtual bool Close() APT_OVERRIDE;
   virtual bool InitHashes(HashStringList const &ExpectedHashes) APT_OVERRIDE;
   virtual Hashes * GetHashes() APT_OVERRIDE;
   virtual ResultState Die(RequestState &Req) APT_OVERRIDE;
   virtual bool Flush(FileFd * const File) APT_OVERRIDE;
   virtual ResultState Go(bool ToFile, RequestState &Req) APT_OVERRIDE;

   HttpServerState(URI Srv, BaseHttpMethod *Owner);
   virtual ~HttpServerState() {Close();};
};


#endif /* HTTPSERVERSTATE_H_ */
