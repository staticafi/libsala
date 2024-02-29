#ifndef SALA_STREAMING_HPP_INCLUDED
#   define SALA_STREAMING_HPP_INCLUDED

#   include <iosfwd>

namespace sala::detail { void enable_json_comments(bool state); }

namespace sala {


struct Program;


template<typename ElementType, typename Traits>
std::basic_ostream<ElementType, Traits>& enable_json_comments(std::basic_ostream<ElementType, Traits>& ostr)
{ detail::enable_json_comments(true); return ostr; }

template<typename ElementType, typename Traits>
std::basic_ostream<ElementType, Traits>& disable_json_comments(std::basic_ostream<ElementType, Traits>& ostr)
{ detail::enable_json_comments(false); return ostr; }


std::ostream& operator<<(std::ostream& ostr, Program const& program);
std::istream& operator>>(std::istream& istr, Program& program);


}

#endif
