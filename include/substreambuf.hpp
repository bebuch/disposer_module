//-----------------------------------------------------------------------------
// Copyright (c) 2009-2015 Benjamin Buch
//
// https://github.com/bebuch/disposer
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)
//-----------------------------------------------------------------------------
#ifndef _disposer_module_substreambuf_hpp_INCLUDED_
#define _disposer_module_substreambuf_hpp_INCLUDED_

#include <streambuf>


namespace disposer_module{


	class substreambuf: public std::streambuf{
	public:
		substreambuf(std::streambuf& buffer, int start, int size):
			buffer_(buffer),
			start_(start),
			size_(size),
			pos_(0)
		{
			buffer_.pubseekpos(start);
			setbuf(nullptr, 0);
		}

	protected:
		virtual int underflow()override{
			if(pos_ + 1 >= size_) return traits_type::eof();

			return buffer_.sgetc();
		}

		virtual int uflow()override{
			if(pos_ + 1 >= size_) return traits_type::eof();

			++pos_;

			return buffer_.sbumpc();
		}

		virtual std::streampos seekoff(std::streamoff off, std::ios_base::seekdir dir, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)override{
			if(dir == std::ios_base::beg){
				pos_ = start_ + off;
			}else if(dir == std::ios_base::cur){
				pos_ += off;
			}else if(dir == std::ios_base::end){
				pos_ = size_ + off;
			}

			if(pos_ < 0 || pos_ >= size_) return -1;
			if(buffer_.pubseekpos(start_ + pos_, std::ios_base::beg, which) == -1) return -1;

			return pos_;
		}

		virtual std::streampos seekpos(std::streampos pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)override{
			pos_ += pos;

			if(pos_ < 0 || pos_ >= size_) return -1;
			if(buffer_.pubseekpos(pos,which) == -1) return -1;

			return pos_;
		}

	private:
		std::streambuf& buffer_;
		std::streampos start_;
		std::streamsize size_;
		std::streampos pos_;
	};


}


#endif
