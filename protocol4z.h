/*
 * Protocol4z License
 * -----------
 * 
 * Protocol4z is licensed under the terms of the MIT license reproduced below.
 * This means that Protocol4z is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2013-2013 YaweiZhang <yawei_zhang@foxmail.com>.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * ===============================================================================
 * 
 * (end of COPYRIGHT)
 */


/*
 * AUTHORS:  YaweiZhang <yawei_zhang@foxmail.com>
 * VERSION:  0.2
 * PURPOSE:  A lightweight library for process protocol .
 * CREATION: 2013.07.04
 * LCHANGE:  2013.12.03
 * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
 */

/*
 * Web Site: www.zsummer.net
 * mail: yawei_zhang@foxmail.com
 */

/* 
 * UPDATES LOG
 * 
 * VERSION 0.1.0 <DATE: 2013.07.4>
 * 	create the first project.  
 * 	support big-endian or little-endian
 * 
 */
#pragma once
#ifndef _PROTOCOL4Z_H_
#define _PROTOCOL4Z_H_

#include <string.h>
#include <string>
#include <assert.h>
#ifndef WIN32
#include <stdexcept>
#else
#include <exception>
#endif
#ifndef _ZSUMMER_BEGIN
#define _ZSUMMER_BEGIN namespace zsummer {
#endif  
#ifndef _ZSUMMER_PROTOCOL4Z_BEGIN
#define _ZSUMMER_PROTOCOL4Z_BEGIN namespace protocol4z {
#endif
_ZSUMMER_BEGIN
_ZSUMMER_PROTOCOL4Z_BEGIN

enum ZSummer_EndianType
{
	BigEndian,
	LittleEndian,
};

//!get runtime local endian type. 
static const unsigned short gc_localEndianType = 1;
inline ZSummer_EndianType LocalEndianType()
{
	if (*(const unsigned char *)&gc_localEndianType == 1)
	{
		return LittleEndian;
	}
	return BigEndian;
}

struct DefaultStreamHeadTrait
{
	typedef unsigned short Integer; 
	const static Integer PreOffset = 0; //前置偏移字节数
	const static Integer PackLenSize = sizeof(Integer); //存储包长的内存字节数
	const static Integer PostOffset = 0; //后置偏移字节数
	const static Integer HeadLen = PreOffset + PackLenSize + PostOffset; //头部总长
	const static bool PackLenIsContainHead = true; // 包长是否包括头部(不包含表示其长度为包体总长)
	const static ZSummer_EndianType EndianType = LittleEndian; //序列化时对整形的字节序
};



template<class Integer, class StreamHeadTrait>
Integer StreamToInteger(const char stream[sizeof(Integer)])
{
	unsigned short integerLen = sizeof(Integer);
	Integer integer = 0 ;
	if (integerLen == 1)
	{
		integer = (Integer)stream[0];
	}
	else
	{
		if (StreamHeadTrait::EndianType != LocalEndianType())
		{
			char *dst = (char*)&integer;
			char *src = (char*)stream + integerLen;
			while (integerLen > 0)
			{
				*dst++ = *++src;
				integerLen --;
			}
		}
		else
		{
			memcpy(&integer, stream, integerLen);
		}
	}
	return integer;
}
template<class Integer, class StreamHeadTrait>
void IntegerToStream(Integer integer, char *stream)
{
	unsigned short integerLen = sizeof(Integer);
	if (integerLen == 1)
	{
		stream[0] = (char)integer;
	}
	else
	{
		if (StreamHeadTrait::EndianType != LocalEndianType())
		{
			char *src = (char*)&integer+integerLen;
			char *dst = (char*)stream;
			while (integerLen > 0)
			{
				*dst++ = *++src;
				integerLen --;
			}
		}
		else
		{
			memcpy(stream, &integer, integerLen);
		}
	}
}


//! return: -1:error,  0:ready, >0: need buff length to recv.
template<class StreamHeadTrait>
inline long long CheckBuffIntegrity(const char * buff, typename StreamHeadTrait::Integer curBuffLen, typename StreamHeadTrait::Integer maxBuffLen)
{
	//! 检查包头是否完整
	if (curBuffLen < StreamHeadTrait::HeadLen)
	{
		return StreamHeadTrait::HeadLen - curBuffLen;
	}

	//! 获取包长度
	StreamHeadTrait::Integer packLen = StreamToInteger<StreamHeadTrait::Integer, StreamHeadTrait>(buff+StreamHeadTrait::PreOffset);
	if (!StreamHeadTrait::PackLenIsContainHead)
	{
		StreamHeadTrait::Integer oldInteger = packLen;
		packLen += StreamHeadTrait::HeadLen;
		if (packLen < oldInteger) //over range
		{
			return -1;
		}
	}

	//! check
	if (packLen > maxBuffLen)
	{
		return -1;
	}
	if (packLen == curBuffLen)
	{
		return 0;
	}
	if (packLen < curBuffLen)
	{
		return -1;
	}
	return packLen - curBuffLen;
}


template<class StreamHeadTrait>
class Stream
{
public:
	Stream()
	{
		m_dataLen = 0;
		m_cursor = StreamHeadTrait::HeadLen;
	}
	Stream(typename StreamHeadTrait::Integer dataLen, typename StreamHeadTrait::Integer cursor)
	{
		m_dataLen = dataLen;
		m_cursor = cursor;
	}
	virtual ~Stream(){}
protected:
	typename StreamHeadTrait::Integer m_dataLen;
	typename StreamHeadTrait::Integer m_cursor;
};

template<class StreamHeadTrait=DefaultStreamHeadTrait, class AllocType = std::allocator<char> >
class WriteStream :public Stream<StreamHeadTrait>
{
public:
	WriteStream()
	{
		m_data.resize(StreamHeadTrait::HeadLen, '\0');
		m_cursor = StreamHeadTrait::HeadLen;
		m_dataLen = (typename StreamHeadTrait::Integer) m_data.length();
	}
	~WriteStream(){}
public:
	//!
	inline void FixPackLen()
	{
		typename StreamHeadTrait::Integer packLen =m_cursor;
		if (!StreamHeadTrait::PackLenIsContainHead)
		{
			packLen -= StreamHeadTrait::HeadLen;
		}
		IntegerToStream<StreamHeadTrait::Integer, StreamHeadTrait>(packLen, &m_data[StreamHeadTrait::PreOffset]);
	}
	inline void GetPackHead(char *& packHead)
	{
		memcpy(packHead,&m_data[0], StreamHeadTrait::HeadLen);
	}
	template <class T>
	inline WriteStream & WriteIntegerData(T t, unsigned short len = sizeof(T))
	{
		m_data.append((const char*)&t, len);
		if (StreamHeadTrait::EndianType != LocalEndianType())
		{
			IntegerToStream<T,StreamHeadTrait>(t, &m_data[m_cursor]);
		}
		m_cursor += len;
		FixPackLen();
		return * this;
	}

	template <class T>
	inline WriteStream & WriteSimpleData(T t, unsigned short len = sizeof(T))
	{
		m_data.append((const char*)&t, len);
		m_cursor += len;
		FixPackLen();
		return * this;
	}
	inline WriteStream & WriteContentData(const void * data, typename StreamHeadTrait::Integer len)
	{
		m_data.append((const char*)data, len);
		m_cursor += len;
		FixPackLen();
		return *this;
	}

	inline char* GetWriteStream(){return &m_data[0];}
	inline typename StreamHeadTrait::Integer GetWriteLen(){return m_cursor;}

	inline WriteStream & operator << (bool data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (char data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (unsigned char data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (short data) { return WriteIntegerData(data);}
	inline WriteStream & operator << (unsigned short data) { return WriteIntegerData(data);}
	inline WriteStream & operator << (int data) { return WriteIntegerData(data);}
	inline WriteStream & operator << (unsigned int data) { return WriteIntegerData(data);}
 	inline WriteStream & operator << (long data) { return WriteIntegerData((long long)data);}
 	inline WriteStream & operator << (unsigned long data) { return WriteIntegerData((unsigned long long)data);}
	inline WriteStream & operator << (long long data) { return WriteIntegerData(data);}
	inline WriteStream & operator << (unsigned long long data) { return WriteIntegerData(data);}
	inline WriteStream & operator << (float data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (double data) { return WriteSimpleData(data);}
	inline WriteStream & operator << (const char *const data) 
	{
		typename StreamHeadTrait::Integer len = (typename StreamHeadTrait::Integer)strlen(data);
		WriteIntegerData(len);
		WriteContentData(data, len);
		return *this;
	}
	inline WriteStream & operator << (const std::string & data) { return *this << data.c_str();}
private:
	std::basic_string<char, std::char_traits<char>, AllocType> m_data;
};



template<class StreamHeadTrait=DefaultStreamHeadTrait>
class ReadStream :public Stream<StreamHeadTrait>
{
public:
	ReadStream(const char *buf, typename StreamHeadTrait::Integer len)
	{
		m_data = buf;
		m_dataLen = len;
		m_cursor = StreamHeadTrait::HeadLen;
	}
	~ReadStream(){}
public:
	inline void CheckMoveCursor(typename StreamHeadTrait::Integer unit)
	{
		if (m_cursor>= m_dataLen)
		{
			throw std::runtime_error("buff is full of");
		}
		if (unit > m_dataLen)
		{
			throw std::runtime_error("unit is too large");
		}
		if (m_dataLen - m_cursor < unit)
		{
			throw std::runtime_error("out of buff");
		}
	}
	inline void MoveCursor(typename StreamHeadTrait::Integer unit)
	{
		m_cursor += unit;
	}
	template <class T>
	inline ReadStream & ReadIntegerData(T & t, unsigned short len = sizeof(T))
	{
		CheckMoveCursor(len);
		t = StreamToInteger<T, StreamHeadTrait>(&m_data[m_cursor]);
		MoveCursor(len);
		return * this;
	}
	template <class T>
	inline ReadStream & ReadSimpleData(T & t, unsigned short len = sizeof(T))
	{
		CheckMoveCursor(len);
		memcpy(&t,&m_data[m_cursor],len);
		MoveCursor(len);
		return * this;
	}
	inline const char * PeekContentData(typename StreamHeadTrait::Integer len)
	{
		CheckMoveCursor(len);
		return &m_data[m_cursor];
	}
	inline void SkipContentData(typename StreamHeadTrait::Integer len)
	{
		CheckMoveCursor(len);
		MoveCursor(len);
	}
	inline ReadStream & ReadContentData(char * data, typename StreamHeadTrait::Integer len)
	{
		memcpy(data, PeekContentData(len), len);
		SkipContentData(len);
		return *this;
	}
public:
	inline typename StreamHeadTrait::Integer GetReadLen(){return m_cursor;}
	inline ReadStream & operator >> (bool & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (char & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (unsigned char & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (short & data) { return ReadIntegerData(data);}
	inline ReadStream & operator >> (unsigned short & data) { return ReadIntegerData(data);}
	inline ReadStream & operator >> (int & data) { return ReadIntegerData(data);}
	inline ReadStream & operator >> (unsigned int & data) { return ReadIntegerData(data);}
 	inline ReadStream & operator >> (long & data)
	{ 
		long long tmp = 0;
		ReadStream & ret = ReadIntegerData(tmp);
		data =(long) tmp;
		return ret;
	}
 	inline ReadStream & operator >> (unsigned long & data)
	{ 
		unsigned long long tmp = 0;
		ReadStream & ret = ReadIntegerData(tmp);
		data = (unsigned long)tmp;
		return ret;
	}
	inline ReadStream & operator >> (long long & data) { return ReadIntegerData(data);}
	inline ReadStream & operator >> (unsigned long long & data) { return ReadIntegerData(data);}
	inline ReadStream & operator >> (float & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (double & data) { return ReadSimpleData(data);}
	inline ReadStream & operator >> (std::string & data) 
	{
		typename StreamHeadTrait::Integer len = 0;
		ReadSimpleData(len);
		const char * p = PeekContentData(len);
		data.assign(p, len);
		SkipContentData(len);
		return *this;
	}
private:
	const char * m_data;
};


#ifndef _ZSUMMER_END
#define _ZSUMMER_END }
#endif  
#ifndef _ZSUMMER_PROTOCOL4Z_END
#define _ZSUMMER_PROTOCOL4Z_END }
#endif

_ZSUMMER_PROTOCOL4Z_END
_ZSUMMER_END

#endif
