#ifndef __TYPE_H__
#define __TYPE_H__

#define IO_ADDRESS(x)           (x)
#if 0
#define readb(addr)             ({U8  _v = ReadIO8(addr);  _v;})
#define readw(addr)             ({U16 _v = ReadIO16(addr); _v;})
#define readl(addr)             ({U32 _v = ReadIO32(addr); _v;})
#else
#define readb(addr)             ReadIO8(addr)
#define readw(addr)             ReadIO16(addr)
#define readl(addr)             ReadIO32(addr)
#endif
#if 0
#define writeb(data, addr)      ({U8  *_v = (U8 *)addr;  WriteIO8(_v, data);})
#define writew(data, addr)      ({U16 *_v = (U16 *)addr; WriteIO16(_v, data);})
#define writel(data, addr)      ({U32 *_v = (U32 *)addr; WriteIO32(_v, data);})
#else
#define writeb(data, addr)      WriteIO8(addr, data)
#define writew(data, addr)      WriteIO16(addr, data)
#define writel(data, addr)      WriteIO32(addr, data)
#endif
#define u8                      U8
#define u16                     U16
#define u32                     U32
#define u64                     U64

#define s8                      S8
#define s16                     S16
#define s32                     S32
#define s64                     S64
#endif
