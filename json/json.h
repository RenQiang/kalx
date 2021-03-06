// json.h - Lightweight C++ wrappers for mongo C library.
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <utility>
#ifndef ensure
#include <cassert>
#define ensure assert
#endif // ensure

using namespace std::rel_ops;

typedef enum {
	JSON_STRING,
	JSON_NUMBER, // double
	JSON_OBJECT,
	JSON_ARRAY,
	JSON_TRUE,
	JSON_FALSE,
	JSON_NULL,
	// extensions for BSON
#ifndef JSON_ONLY
	JSON_BYTE,
	JSON_INT32,
	JSON_INT64,
	JSON_DATE,
#endif
	JSON_UNDEFINED // "empty" type
} json_element_type;

namespace json {

	class value;
	struct element;
	typedef std::pair<std::string,json::value> pair;
	typedef std::map<std::string, value> object;

	// POD types for holding the bits
	struct string {
		size_t size;
		const char* data;
	};
	inline string string_(size_t size = 0, const char* data = 0)
	{
		string s;

		s.size = size;
		s.data = data;

		return s;
	}

	struct array {
		size_t size;
		json::element* element;
	};
	inline array array_(size_t size, json::element* element)
	{
		array a;

		a.size = size;
		a.element = element;

		return a;
	}
#ifndef JSON_ONLY
	struct byte {
		size_t size;
		const uint8_t* data;
	};
	inline byte byte_(size_t size, const uint8_t* data)
	{
		byte b;

		b.size = size;
		b.data = data;

		return b;
	}
#endif
	struct element {
		union {
			json::string string;
			double number;
			json::object* object;
			json::array array;
#ifndef JSON_ONLY
			json::byte byte;
			int32_t int32;
			int64_t int64;
			time_t date;
#endif
		} data;
		json_element_type type;
	};

	inline bool operator==(const string& s, const string& t)
	{
		return s.size == t.size && 0 == strcmp(s.data, t.data);
	}
	inline bool operator==(const string& s, const char* t)
	{
		return s == string_(strlen(t), t);
	}
	inline bool operator<(const string& s, const string& t)
	{
		return strcmp(s.data, t.data) < 0;
	}
	inline bool operator<(const string& s, const char* t)
	{
		return s < string_(strlen(t), t);
	}

	inline bool operator==(const array& a, const array& b)
	{
		return a.size == b.size && std::equal(a.element, a.element + a.size, b.element);
	}
	inline bool operator<(const array& a, const array& b)
	{
		return std::lexicographical_compare(a.element, a.element + a.size, b.element, b.element + b.size);
	}

	inline bool operator==(const byte& a, const byte& b)
	{
		return a.size == b.size && 0 == memcmp(a.data, b.data, a.size);
	}
	inline bool operator<(const byte& a, const byte& b)
	{
		return std::lexicographical_compare(a.data, a.data + a.size, b.data, b.data + b.size);
	}

	inline bool operator==(const element& e, const string& s)
	{
		return e.type == JSON_STRING && e.data.string == s;
	}
	inline bool operator<(const element& e, const string& s)
	{
		return e.type == JSON_STRING && e.data.string < s;
	}
	inline bool operator==(const element& e, const char* s)
	{
		return e.type == JSON_STRING && e.data.string == s;
	}
	inline bool operator<(const element& e, const char* s)
	{
		return e.type == JSON_STRING && e.data.string < s;
	}

	inline bool operator==(const element& e, double number)
	{
		return e.type == JSON_NUMBER && e.data.number == number;
	}
	inline bool operator<(const element& e, double number)
	{
		return e.type == JSON_NUMBER && e.data.number < number;
	}

	inline bool operator==(const element& e, const array& array)
	{
		return e.type == JSON_ARRAY && e.data.array == array;
	}
	inline bool operator<(const element& e, const array& array)
	{
		return e.type == JSON_ARRAY && e.data.array < array;
	}
#ifndef JSON_ONLY
	inline bool operator==(const element& e, const byte& byte)
	{
		return e.type == JSON_BYTE && e.data.byte == byte;
	}
	inline bool operator<(const element& e, const byte& byte)
	{
		return e.type == JSON_BYTE && e.data.byte < byte;
	}
#endif
	inline bool operator==(const element& e, bool b)
	{
		return (b && e.type == JSON_TRUE) || (!b && e.type == JSON_FALSE);
	}
	inline bool operator<(const element& e, bool b)
	{
		return e.type == JSON_FALSE && b;
	}
#ifndef JSON_ONLY
	inline bool operator==(const element& e, time_t date)
	{
		return e.type == JSON_DATE && e.data.date == date;
	}
	inline bool operator<(const element& e, time_t date)
	{
		return e.type == JSON_DATE && e.data.date < date;
	}
#endif
	inline bool operator==(const element& a, const element& b)
	{
		return a.type != b.type ? false
			: a.type == JSON_STRING ? a == b.data.string
			: a.type == JSON_NUMBER ? a == b.data.number
			: a.type == JSON_OBJECT ? a.data.object == b.data.object
			: a.type == JSON_ARRAY ? a == b.data.array
			: a.type == JSON_TRUE ? b.type == JSON_TRUE
			: a.type == JSON_FALSE ? b.type == JSON_FALSE
			: a.type == JSON_NULL ? false // just like javascript
#ifndef JSON_ONLY
			: a.type == JSON_BYTE ? a == b.data.byte
			: a.type == JSON_INT32 ? a.data.int32 == b.data.int32 
			: a.type == JSON_INT64 ? a.data.int64 == b.data.int64
			: a.type == JSON_DATE ? a.data.date == b.data.date
#endif
			: false
			;
	}
	inline bool operator<(const element& a, const element& b)
	{
		return a.type < b.type ? true
			: a.type >  b.type ? false
			: a.type == JSON_STRING ? a < b.data.string
			: a.type == JSON_NUMBER ? a < b.data.number
			: a.type == JSON_OBJECT ? a.data.object < b.data.object
			: a.type == JSON_ARRAY ? a < b.data.array
			: a.type == JSON_TRUE ? false
			: a.type == JSON_FALSE ? b.type == JSON_TRUE
			: a.type == JSON_NULL ? false // just like javascript
#ifndef JSON_ONLY
			: a.type == JSON_BYTE ? a < b.data.byte
			: a.type == JSON_INT32 ? a.data.int32 < b.data.int32 
			: a.type == JSON_INT64 ? a.data.int64 < b.data.int64
			: a.type == JSON_DATE ? a.data.date < b.data.date
#endif
			: false
			;
	}

	// real class for managing memory
	class value : public element {
	public:
		operator json::element&()
		{
			return *this;
		}
		operator const json::element&() const
		{
			return *this;
		}
		operator bool() const
		{
			return type != JSON_UNDEFINED;
		}

		value()
		{
			type = JSON_UNDEFINED;
		}
		value(const value& v)
		{
			operator=(v);
		}
		value& operator=(const value& v)
		{
			if (this != &v) {
				switch (v.type) {
				case JSON_STRING:
					operator=(v.data.string);
					break;
				case JSON_ARRAY:
					operator=(v.data.array);
					break;
#ifndef JSON_ONLY
				case JSON_BYTE:
					operator=(v.data.byte);
					break;
#endif
				default: // non pointer types
					type = v.type;
					data = v.data;
				}
			}

			return *this;
		}
		value(const json::element& e)
		{
			operator=(e);
		}
		value& operator=(const json::element& e)
		{
			switch (e.type) {
			case JSON_STRING:
				operator=(e.data.string);
				break;
			case JSON_ARRAY:
				operator=(e.data.array);
				break;
#ifndef JSON_ONLY
			case JSON_BYTE:
				operator=(e.data.byte);
				break;
#endif
			default: // non pointer types
				type = e.type;
				data = e.data;
			}

			return *this;
		}
		~value()
		{
			delete_value();
		}

		bool operator==(const value& v) const
		{
			return this->operator const json::element &() == v.operator const json::element &();
		}

		// string
		value(const char* s)
		{
			construct_string(s);
		}
		value(const json::string& s)
		{
			construct_string(s.data, s.size);
		}
		value& operator=(const char* s)
		{
			delete_value();
			construct_string(s);

			return *this;
		}
		value& operator=(const json::string& s)
		{
			delete_value();
			construct_string(s.data, s.size);

			return *this;
		}
		// specialize for const char*
		bool operator==(const char* s) const
		{
			return operator const json::element&() == s;
		}
		bool operator<(const char* s) const
		{
			return operator const json::element&() < s;
		}

		// number
		explicit value(double number)
		{
			type = JSON_NUMBER;
			data.number = number;
		}
		value& operator=(double number)
		{
			delete_value();
			type = JSON_NUMBER;
			data.number = number;

			return *this;
		}
		bool operator==(double number) const
		{
			return operator const json::element&() == number;
		}
		bool operator<(double number) const
		{
			return operator const json::element&() < number;
		}

		// array
		explicit value(int n)
		{
			construct_array(n);
		}
		value& operator=(const array& a)
		{
			delete_value();
			construct_array(a.size);
			for (size_t i = 0; i < a.size; ++i)
				operator[](i) = a.element[i];

			return *this;
		}
		json::value& operator[](size_t i)
		{
			// ensure type == JSON_ARRAY;
			return static_cast<json::value&>(data.array.element[i]);
		}
		const json::value& operator[](size_t i) const
		{
			// ensure type == JSON_ARRAY;
			return static_cast<const json::value&>(data.array.element[i]);
		}
		json::value& push_back(const json::element& element)
		{
			push_back_array(element);
			
			return *this;
		}
		json::value& push_back(const json::array& array)
		{
			push_back_array(array);
			
			return *this;
		}
#ifndef JSON_ONLY
		// byte
		value(size_t size, uint8_t* data)
		{
			construct_byte(size, data);
		}
		value& operator=(const byte& b)
		{
			delete_value();
			construct_byte(b.size, b.data);

			return *this;
		}
#endif
		explicit value(bool b)
		{
			type = b ? JSON_TRUE : JSON_FALSE;
		}
		value& operator=(bool b)
		{
			delete_value();
			type = b ? JSON_TRUE : JSON_FALSE;

			return *this;
		}
		bool operator==(bool b) const
		{
			return operator const json::element&() == b;
		}
		bool operator<(bool b) const
		{
			return operator const json::element&() < b;
		}
#ifndef JSON_ONLY
		// int32
		// int64

		// time_t
		explicit value(time_t t)
		{
			type = JSON_DATE;
			data.date = t;
		}
		value& operator=(time_t t)
		{
			delete_value();
			type = JSON_DATE;
			data.date = t;

			return *this;
		}
		bool operator==(time_t date) const
		{
			return operator const json::element&() == date;
		}
		bool operator<(time_t date) const
		{
			return operator const json::element&() < date;
		}
#endif
	protected:
		void construct_string(const char* s, size_t size = 0)
		{
			type = JSON_STRING;
			data.string.size = size ? size : strlen(s);
			data.string.data = new char[data.string.size + 1];
			strcpy(const_cast<char*>(data.string.data), s);
		}
		void delete_string(void)
		{
			delete [] data.string.data;
			type = JSON_UNDEFINED;
		}

		void construct_array(size_t n)
		{
			type = JSON_ARRAY;
			data.array.size = n;
			data.array.element = static_cast<json::element*>(malloc(n*sizeof(json::element)));
			for (size_t i = 0; i < n; ++i)
				data.array.element[i].type = JSON_UNDEFINED;
		}
		void delete_array(void)
		{
			for (size_t i = 0; i < data.array.size; ++i)
				operator[](i).delete_value();
			
			free(data.array.element);

			type = JSON_UNDEFINED;
		}
		void push_back_array(const json::element& element)
		{
			if (type == JSON_UNDEFINED) {
				construct_array(1);
				operator[](0) = element;
			}
			else {
				if (type != JSON_ARRAY) {
					value this_(*this);
					delete_value();
					construct_array(2);
					operator[](0) = this_;
					operator[](1) = element;
				}
				else {
					data.array.element = static_cast<json::element*>(realloc(data.array.element, (data.array.size + 1)*sizeof(json::element)));
					operator[](data.array.size) = element;
					++data.array.size;
				}
			}
		}
		void push_back_array(const array& array)
		{
			if (type == JSON_UNDEFINED) {
				operator=(array);
			}
			else {
				if (type != JSON_ARRAY) {
					value this_(*this);
					delete_value();
					construct_array(1);
					operator[](0) = this_;
				}
				data.array.element = static_cast<json::element*>(realloc(data.array.element, (data.array.size + array.size)*sizeof(json::element)));
				for (size_t i = 0; i < array.size; ++i)
					operator[](data.array.size + i) = array.element[i];
				data.array.size += array.size;
			}
		}
#ifndef JSON_ONLY
		void construct_byte(size_t n, const uint8_t* b)
		{
			type = JSON_BYTE;
			data.byte.size = n;
			data.byte.data = new uint8_t[n];
			memcpy(const_cast<uint8_t*>(data.byte.data), b, n);
		}
		void delete_byte(void)
		{
			delete [] data.byte.data;
			type = JSON_UNDEFINED;
		}
#endif
		void delete_value()
		{
			switch (type) {
			case JSON_STRING:
				delete_string();
				break;
			case JSON_ARRAY:
				delete_array();
				break;
#ifndef JSON_ONLY
			case JSON_BYTE:
				delete_byte();
				break;
#endif
			default:
				type = JSON_UNDEFINED;
			}
		}
	};

	namespace parse {
		inline bool eat(char c, std::istream& is)
		{
			char c_;

			is >> std::skipws >> c_;

			return c == c_;
		}
		inline char eat(const char* s, std::istream& is)
		{
			char c_;

			is >> std::skipws >> c_;

			return *strchr(s, c_);
		}

		inline json::value read_value(std::istream& is);

		inline json::value read_array(std::istream& is)
		{
			json::value v;

			while (json::value a = read_value(is)) {
				v.push_back(a);
			}

			return v;
		}
		inline std::string read_string(std::istream& is)
		{
			char c;
			std::string s;

			for (is >> c; c != '\"' && c != '\''; is >> c)
				s += c; // does not handle escaped quotes!!!
			
			return s;
		}
		inline json::value read_value(std::istream& is)
		{
			char c;
			json::value v;

			is >> std::skipws >> c;

			if (c == ']' || c == '}') {
				return v;
			}

			if (c == ',') {
				is >> std::skipws >> c;
			}

			if (c == '[') {
				v = read_array(is);
			}
			else if (c == '\"' || c == '\'')
				v = read_string(is).c_str();
			else if (c == 'f') {
				ensure (eat('a', is));
				ensure (eat('l', is));
				ensure (eat('s', is));
				ensure (eat('e', is));
				v = false;
			}
			else if (c == 't') {
				ensure (eat('r', is));
				ensure (eat('u', is));
				ensure (eat('e', is));
				v = true;
			}
			else if (c == 'n') {
				ensure (eat('u', is));
				ensure (eat('l', is));
				ensure (eat('l', is));
				v.type = JSON_NULL;
			}
			else {
				is.putback(c);
				is >> v.data.number;
				ensure (!is.fail());
				v.type = JSON_NUMBER;
			}

			return v;
		}
		inline std::string read_key(std::istream& is)
		{
			std::string key = read_string(is);

			ensure (parse::eat(':', is));

			return key;
		}
		inline bool read_pair(std::istream& is, std::pair<std::string,json::value>& kv)
		{
			char c;
			is >> std::skipws >> c;

			if (c == '}') {
				return false;
			}

			ensure (c == '\"' || c == '\'');
			kv.first = read_key(is);
			kv.second = read_value(is);

			return true;
		}
		inline object read_members(std::istream& is)
		{
			object o;
			std::pair<std::string,json::value> kv;

			while (read_pair(is, kv)) {
				o.insert(kv);
			}

			return o;
		}

		inline object read_object(std::istream& is)
		{
			ensure (parse::eat('{', is));
			object o = parse::read_members(is);

			return o;
		}

	} // namespace parse

} // namespace bson

std::ostream& operator<<(std::ostream& os, const json::value& v)
{
	switch (v.type) {
	case JSON_STRING: os << '"' << v.data.string.data << '"'; break;
	case JSON_NUMBER: os << v.data.number; break;
	case JSON_OBJECT: os << '(' << v.data.object << ')'; break;
	case JSON_ARRAY: { 
		os << '[';
		for (size_t i = 0; i < v.data.array.size; ++i) {
			if (i) os << ',';
			os << v[i];
		}
		os << ']';
		break;
	}
	case JSON_TRUE:  os << "true"; break;
	case JSON_FALSE: os << "false"; break;
	case JSON_NULL:  os << "null"; break;
	case JSON_BYTE: { for (size_t i = 0; i < v.data.byte.size; ++i) os << v.data.byte.data[i]; } break;
	case JSON_INT32: os << v.data.int32; break;
	case JSON_INT64: os << v.data.int64; break;
	case JSON_DATE: os << v.data.date; break; // pretty print???

	default:
		os << "*undefined*";
	}

	return os;
}
std::ostream& operator<<(std::ostream& os, const json::object& o)
{
	std::map<std::string,json::value>::const_iterator i;

	os << '{';
	for (i = o.begin(); i != o.end(); ++i) {
		if (i != o.begin()) os << ',';
		os << '"' << i->first << "\":" << i->second; 
	}
	os << '}';

	return os;
}

std::istream& operator>>(std::istream& is, json::value& v)
{
	v = json::parse::read_value(is);

	return is;
}
std::istream& operator>>(std::istream& is, json::object& o)
{
	o = json::parse::read_object(is);

	return is;
}



