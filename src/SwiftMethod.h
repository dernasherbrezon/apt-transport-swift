// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   HTTP Acquire Method - This is the HTTP acquire method for APT.

   ##################################################################### */
									/*}}}*/

#ifndef APT_HTTP_H
#define APT_HTTP_H

#include <apt-pkg/strutl.h>

#include <iostream>
#include <memory>
#include <string>
#include <sys/time.h>

#include "basehttp.h"
#include "connect.h"
#include "CircleBuf.h"

using std::cout;
using std::endl;

class FileFd;
class SwiftMethod;

class SwiftMethod : public BaseHttpMethod
{
   public:
   virtual void SendReq(FetchItem *Itm) APT_OVERRIDE;

   virtual std::unique_ptr<ServerState> CreateServerState(URI const &uri) APT_OVERRIDE;
   virtual void RotateDNS() APT_OVERRIDE;
   virtual DealWithHeadersResult DealWithHeaders(FetchResult &Res, RequestState &Req) APT_OVERRIDE;

   protected:
   std::string AutoDetectProxyCmd;

   public:
   friend struct HttpServerState;

   explicit SwiftMethod(std::string &&pProg);
};

#endif
