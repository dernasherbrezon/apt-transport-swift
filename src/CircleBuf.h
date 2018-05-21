#ifndef CIRCLEBUF_H_
#define CIRCLEBUF_H_

#include <string>
#include <memory>
#include <apt-pkg/hashes.h>

#include "connect.h"

class CircleBuf
{
   unsigned char *Buf;
   unsigned long long Size;
   unsigned long long InP;
   unsigned long long OutP;
   std::string OutQueue;
   unsigned long long StrPos;
   unsigned long long MaxGet;
   struct timeval Start;

   static unsigned long long BwReadLimit;
   static unsigned long long BwTickReadData;
   static struct timeval BwReadTick;
   static const unsigned int BW_HZ;

   unsigned long long LeftRead() const
   {
      unsigned long long Sz = Size - (InP - OutP);
      if (Sz > Size - (InP%Size))
	 Sz = Size - (InP%Size);
      return Sz;
   }
   unsigned long long LeftWrite() const
   {
      unsigned long long Sz = InP - OutP;
      if (InP > MaxGet)
	 Sz = MaxGet - OutP;
      if (Sz > Size - (OutP%Size))
	 Sz = Size - (OutP%Size);
      return Sz;
   }
   void FillOut();

   public:
   Hashes *Hash;
   // total amount of data that got written so far
   unsigned long long TotalWriten;

   // Read data in
   bool Read(std::unique_ptr<MethodFd> const &Fd);
   bool Read(std::string const &Data);

   // Write data out
   bool Write(std::unique_ptr<MethodFd> const &Fd);
   bool Write(std::string &Data);
   bool WriteTillEl(std::string &Data,bool Single = false);

   // Control the write limit
   void Limit(long long Max) {if (Max == -1) MaxGet = 0-1; else MaxGet = OutP + Max;}
   bool IsLimit() const {return MaxGet == OutP;};
   void Print() const {std::cout << MaxGet << ',' << OutP << std::endl;};

   // Test for free space in the buffer
   bool ReadSpace() const {return Size - (InP - OutP) > 0;};
   bool WriteSpace() const {return InP - OutP > 0;};

   void Reset();
   // Dump everything
   void Stats();

   CircleBuf(int downloadLimit, unsigned long long Size);
   ~CircleBuf();
};


#endif /* CIRCLEBUF_H_ */
