/*
 * format.h
 *
 *  Created on: 18. 12. 2021
 *      Author: ondra
 */

#ifndef LOG4HPP_FORMAT_H_
#define LOG4HPP_FORMAT_H_


/**
 *
 *  "Message"
 *  "Message with arg1={} and arg2={} and arg3={}"
 *  "Message with positions arg2={2} and arg1={1} and arg3={3}"
 *  "Message with positions and format-spec arg2={2:urlencode} and arg1={1:aaa}" -
 *  "Message with {{}}"
 *
 *  Mixing
 *
 */

namespace log4hpp {

template<typename T> class Stringify;


template<typename Out, typename MapType>
class FormatT {
public:

	using OutRef = std::remove_reference_t<Out> &;

	FormatT(Out &&out, MapType &&map = MapType()):map(std::forward<MapType>(map)),out(std::forward<Out>(out)) {}

	template<typename ... Args>
	void operator()(const std::string_view &format, const Args & ... args);


protected:
	MapType map;
	Out out;


	template<typename T>
	auto formatItem(const std::string_view &format_spec, const T &val) -> decltype(std::declval<Stringify<decltype(std::declval<T>()())> >()(std::declval<T>()(), std::declval<std::string_view>(), std::declval<OutRef &>())){
		Stringify<decltype(std::declval<T>()())> s;
		s(val(),format_spec, out);
	}
	template<typename T>
	auto formatItem(const std::string_view &format_spec, const T &val) -> decltype(std::declval<Stringify<T> >()(std::declval<T>(), std::declval<std::string_view>(), std::declval<OutRef &>())){
		Stringify<T> s;
		s(val,format_spec, out);
	}

	template<typename T, typename ... Args>
	void formatNth(const std::string_view &format_spec, unsigned int nth, const T &val, const Args & ... args) {
		if (nth == 1) {
			formatItem(format_spec, val);
		} else {
			formatNth(format_spec, nth-1, args...);
		}
	}

	void formatNth(const std::string_view &format_spec,unsigned int nth) {
		out.append("{?}");
	};

	template<typename T, typename ... Args>
	void formatNext(const std::string_view &fmt_spec, const std::string_view &format, const T &val, const Args & ... args) {
		formatItem(fmt_spec, val);
		operator()(format, args...);
	}

	void formatNext(const std::string_view &fmt_spec, const std::string_view &format)  {
		out.append("{+}");
		operator()(format);
	};

	static std::size_t find_subfmt_def(const std::string_view &txt, std::size_t pos, char end) {
		while (pos < txt.size()) {
			char c = txt[pos];
			if (c == end) return pos;
			switch (c) {
				case '(': pos = find_subfmt_def(txt,pos+1,')');break;
				case '{': pos = find_subfmt_def(txt,pos+1,'}');break;
				case '[': pos = find_subfmt_def(txt,pos+1,']');break;
				default: break;
			}
			++pos;
		}
		return txt.npos-1;

	}

	static std::size_t find_fmt_def(const std::string_view &txt, std::size_t pos) {
		while (pos < txt.size()) {
			char c = txt[pos];
			switch (c) {
			case ':':
			case '}': return pos;
			case '(': pos = find_subfmt_def(txt,pos+1,')');break;
			case '{': pos = find_subfmt_def(txt,pos+1,'}');break;
			case '[': pos = find_subfmt_def(txt,pos+1,']');break;
			default: break;
			}
			++pos;
		}
		return txt.npos;
	}
};


template<typename Out, typename MapType>
template<typename ... Args>
inline void FormatT<Out, MapType>::operator ()(const std::string_view &format, const Args & ... args) {
	std::size_t p = 0;
	std::size_t len = format.length();
	while (p < len) {
		char c = format[p];
		if (c == '{' && p+1<len) {
			char d = format[p+1];
			if (isdigit(d) || d == ':') {
				unsigned int idx = 0;
				p++;
				while (p < len && isdigit(c=format[p])) {
					idx = idx * 10 + (c-'0');
					++p;
				}
				if (p<len) {
					if (c == ':') {
						p++;
						auto pos = format.find('}', p);
						if (pos == format.npos) return;
						auto fmt = format.substr(p,pos-p);
						if (idx == 0) return formatNext(fmt, format.substr(pos+1), args...);
						formatNth(fmt, idx, args...);
						p = pos+1;
					} else {
						if (idx == 0) return formatNext(std::string_view(), format.substr(p),  args...);
						formatNth(std::string_view(), idx, args...);
					}
				} else {
					break;
				}
			} else if (isalpha(d)) {
				++p;
				auto pend = find_fmt_def(format, p);
				if (pend == format.npos) return;
				std::string_view name = format.substr(p,pend-p);
				std::string_view fmt_spec;
				if (format[pend] == ':') {
					p = pend+1;
					pend = format.find('}', p);
					if (pend == format.npos) return;
					fmt_spec = format.substr(p,pend-p);
				}
				p = pend;
				map(name,[&](const auto &val){
					formatItem(fmt_spec, val);
				});
			} else if (d == '}') {
				return formatNext(std::string_view(), format.substr(p+2), args...);
			} else if (d == c) {
				++p;
				out.push_back(c);
			} else {
				out.push_back(c);
			}
		} else if (c == '}'  && p+1<len) {
			char d = format[p+1];
			if (d == c) {
				out.push_back(c);
				p++;
			} else {
				out.push_back(c);
			}
		} else {
			out.push_back(c);
		}
		p++;
	}
}


class NullMap {
public:
	enum Unknown {unknown};

	template<typename Fn>
	void operator()(const std::string_view &name, Fn &&fn) {
		fn(unknown);
	}
};


class StringifyUnsigned {
public:

	struct Cfg {
		int base;
		int chsofs;
	};

	template<typename T, typename Fn>
	static void writeNumber(const T &val, int zeroes, int base, Fn &&out) {
		if (val || zeroes>0) {
			writeNumber(val/base, zeroes-1, base, std::forward<Fn>(out));
			auto p = val%base;
			char c;
			if (p < 10) c = '0'+p;
			else if (p < 36) c = 'A'+(p-10);
			else c = 'a'+(p-36);
			out(c);
		}
	}


	template<typename T, typename Out>
	void operator()(const T &val, const std::string_view &fmt, Out &out) {
		int base = 10;
		int zeroes = 0;
		bool tolower = false;
		for (char c: fmt) {
			if (isdigit(c)) zeroes = zeroes * 10 + (c - '0');
			else if (c == 'x') {base=16;tolower = true;}
			else if (c == 'X') {base=16;}
			else if (c == 'A') {base=62;}
			else if (c == 'a') {base=32;}
			else if (c == 'o' || c== 'O') {base=8;}
			else if (c == 'b' || c== 'B') {base=2;}
		}
		if (zeroes<1) zeroes = 1;
		if (tolower) {writeNumber(val, zeroes, base, [&](char c){
			out(std::tolower(c));
		});
		} else {
			writeNumber(val, zeroes, base, [&](char c){
					out(c);
			});
		}

	}
};

class StringifySigned {
public:
	template<typename T, typename Out>
	void operator()(const T &val, const std::string_view &fmt, Out &out) {
		bool neg = val < 0;
		auto v = neg?-val:val;
		StringifyUnsigned us;
		if (!fmt.empty() && fmt[0] == '+') {
			out.push_back(neg?'-':'+');
			us(v, fmt.substr(1), out);
		} else if (!fmt.empty() && fmt[0] == ' ') {
			out.push_back(neg?'-':' ');
			us(v, fmt.substr(1), out);
		} else {
			if (neg) out.push_back('-');
			us(v, fmt, out);
		}
	}
};

class StringifyString {
public:
	template<typename Out>
	void operator()(const std::string_view &val, const std::string_view &fmt, Out &out) {
		bool dots = true;
		int space=0;
		bool escape = false;
		bool quotes = false;
		bool dbl = false;
		char qchar = 0;
		bool align_right = false;
		bool utf8 = false;
		bool bin = false;

		for(char c: fmt) {
			switch (c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': space = space * 10 + (c - '0');
			case 'r': dots = false;utf8=false;escape=false;bin=false;break;
			case 'b': bin = true;break;
			case 'q': quotes = true; qchar='"';break;
			case 'a': quotes = true; qchar='\'';break;
			case 'd': dbl = true;break;
			case 'e': escape = true;dots=false;break;
			case 'j': dots = false;escape=true;utf8=true;quotes=true;qchar='"';dbl = false;break;
			case '-': align_right = true;break;
			case '>': align_right = true;break;
			case '<': align_right = false;break;
			default: break;
			}
		}

		auto process = [&](const std::string_view &str, auto &&out) {
			using OutRef = std::remove_reference_t<decltype(out)>;
			if (quotes) {
				out(qchar);
			}
			int utfn = 0;
			unsigned int uchr = 0;
			for (char c: val) {
				if (bin) {
					StringifyUnsigned::writeNumber(static_cast<unsigned char>(c), 2, 16, std::forward<OutRef>(out));
					continue;
				}
				if (escape) {
					switch (c) {
					case '\f': out('\\');out('f');continue;
					case '\b': out('\\');out('b');continue;
					case '\r': out('\\');out('r');continue;
					case '\n': out('\\');out('n');continue;
					case '\t': out('\\');out('t');continue;
					default: break;
					}
				}
				unsigned char d = static_cast<unsigned char>(c);
				if (d >= 32 && d<128) {
					if (quotes && qchar == c) {
						if (dbl) {
							out(c);
							out(c);
						} else if (escape) {
							out('\\');
							out(c);
						} else if (dots) {
							out('.');
						}
					} else if (c == '\\' && escape) {
						out('\\');
						out('\\');
					} else {
						out(c);
					}
				} else if (d < 32) {
					if (dots) {
						out('.');
					}
					else if (escape) {
						out('\\');
						out('u');
						StringifyUnsigned::writeNumber(d, 4, 16, std::forward<OutRef>(out));
					}
					else {
						out(c);
					}
				} else if (d >= 128) {
					if (utf8) {
						unsigned int n;
						if (d < 0xC0) {
							if (d >= 0x80 && uchr) {
								utfn <<= 7;
								utfn |= d & 0x7F;
								uchr--;
								if (uchr) continue;
							}
							n = utfn;
						} else {
							if ((d & 0xE0) == 0xC0) {uchr = 1; utfn = d & 0x1F;}
							else if ((d & 0xF0) == 0xE0) {uchr =2; utfn = d & 0xF;}
							else if ((d & 0xF8) == 0xF0) {uchr =3; utfn = d & 0x7;}
							else if ((d & 0xFC) == 0xF8) {uchr =4; utfn = d & 0x3;}
							else if ((d & 0xFE) == 0xFC) {uchr =5; utfn = d & 0x1;}
							else if ((d & 0xFF) == 0xFE) {uchr =6; utfn = 0;}
							else {uchr =7; utfn = 0;}
							continue;
						}
						out('\\');
						out('u');
						StringifyUnsigned::writeNumber(n, 4, 16, std::forward<OutRef>(out));
					} else {
						out(c);
					}
				}
			}
			if (quotes) {
				out(qchar);
			}

		};
		int bspace = 0;
		int aspace = 0;
		if (space) {
			int cnt = 0;
			process(val, [&](char){++cnt;});
			if (cnt < space) {
				if (align_right) bspace = space - cnt;
				else aspace = space - cnt;
			}
		}
		for (int i =  0; i <bspace; i++) out(' ');
		process(val,[&](char c){out(c);});
		for (int i =  0; i <aspace; i++) out(' ');
	}
};

class StringifyReal {
public:
	template<typename Out>
	void operator()(double val, const std::string_view &fmt, Out &out) {
		char buff[100];
		unsigned int cnt;
		if (!fmt.empty()) {
			int decimals = 0;
			for (char c: fmt) if (isdigit(c)) decimals=decimals * 10 + (c - '0');
			cnt = snprintf(buff,sizeof(buff), "%.*f", decimals, val);
		} else {
			cnt = snprintf(buff,sizeof(buff), "%g", val);
		}
		if (cnt > sizeof(buff)) cnt = sizeof(buff);
		for (unsigned int i = 0; i < cnt; i++) out(buff[i]);
	}

};

template<> class Stringify<bool> {public: template<typename Out> void operator()(bool val, const std::string_view &fmt, Out &out) {
	for (char c: val?"true":"false") out(c);
}
};

template<> class Stringify<char> {
public:
	template<typename Out>
	void operator()(char val, const std::string_view &fmt, Out &out) {
		if (fmt.empty()) out.push_back(val);
		else {
			StringifySigned ss;
			ss(val, fmt, out);
		}
	}};

template<> class Stringify<unsigned char>: public StringifyUnsigned {};
template<> class Stringify<unsigned int>: public StringifyUnsigned {};
template<> class Stringify<unsigned long>: public StringifyUnsigned {};
template<> class Stringify<unsigned long long>: public StringifyUnsigned {};
template<> class Stringify<signed int>: public StringifySigned {};
template<> class Stringify<signed long>: public StringifySigned {};
template<> class Stringify<signed long long>: public StringifySigned {};
template<> class Stringify<std::string>: public StringifyString {};
template<> class Stringify<std::string_view>: public StringifyString {};
template<> class Stringify<const char *>: public StringifyString {};
template<int n> class Stringify<const char [n]>: public StringifyString {};
template<int n> class Stringify<char [n]>: public StringifyString {};
template<> class Stringify<float>: public StringifyReal {};
template<> class Stringify<double>: public StringifyReal {};

template<> class Stringify<NullMap::Unknown> {
	public: template<typename Out> void operator()(char val, const std::string_view &fmt, Out &out) {
		for (char c:"{%}") out(c);
	}
};

class DirectString: public std::string_view {
public:
	using std::string_view::string_view;
};

template<> class Stringify<DirectString>{
public: template<typename Out> void operator()(const DirectString &val, const std::string_view &, Out &out) {
	for (char c: val) out(c);
}
};


}



#endif /* LOG4HPP_FORMAT_H_ */
