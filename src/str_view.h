#ifndef STR_VIEW_H
#define STR_VIEW_H

#include <algorithm>
#include <string>
#include <type_traits>


template<typename CharT, typename Traits = std::char_traits<CharT>>
class str_view {
public:
    // types
    using traits_type = Traits;
    using value_type = CharT;
    using pointer = CharT*;
    using const_pointer = const CharT*;
    using reference = CharT&;
    using const_reference = const CharT&;
    using const_iterator = pointer; // implementation-defined; see 21.4.2.2
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    // static constexpr size_type npos = size_type(-1);

    // construction and assignment
    str_view() : ptr_(nullptr), len_(0) {}
    str_view(const str_view&) = default;
    str_view& operator=(const str_view&) = default;
    str_view(const CharT* ptr, std::size_t len) : ptr_(ptr), len_(len) {}
    str_view(const CharT* ptr) : ptr_(ptr), len_(Traits::length(ptr)) {}

    // iterator support
    const CharT* begin() const { return ptr_; }
    const CharT* end() const { return ptr_ + len_; }

    // capacity
    std::size_t size() const { return len_; }
    std::size_t length() const { return len_; }
    bool empty() const { return len_ == 0; }

    // element access
    const CharT& operator[](std::size_t ind) const {
        return ptr_[ind];
    }
    const CharT* data() const { return ptr_; }

    // modifiers
    void remove_prefix(size_t n) {
        ptr_ += n;
        len_ -= n;
    }
    void remove_suffix(size_t n) {
        len_ -= n;
    }
    void swap(str_view& x) {
        const str_view tmp{x};
        x = *this;
        *this = tmp;
    }

    // string operations
    int compare(str_view x) const {
        const int cmp = Traits::compare(ptr_, x.ptr_, std::min(len_, x.len_));
        return cmp != 0 ? cmp : (len_ == x.len_ ? 0 : len_ < x.len_ ? -1 : 1);
    }


    // non-standart
    bool equal(str_view x) const {
        return len_ == x.len_ && Traits::compare(ptr_, x.ptr_, len_) == 0;
    }

    // basic_string, basic_string_view support
    template <class StrT, typename = std::enable_if<std::is_same<StrT::value_type, value_type>::value>::type>
    str_view(const StrT& str) : ptr_(str.data()), len_(str.length()) {}

    template <class StrT, typename = std::enable_if<std::is_same<StrT::value_type, value_type>::value>::type>
    operator StrT() const { return StrT(ptr_, len_); }

    // basic_string support
    template <class StrT, typename = std::enable_if<std::is_same<StrT::value_type, value_type>::value>::type>
    void append_to(StrT& str) const {
        str.append(ptr_, len_);
    }

private:
    const CharT* ptr_;
    std::size_t len_;
};


// compare two str_view
template<class CharT, class Traits>
/*constexpr*/ bool operator==(str_view<CharT, Traits> lhs, str_view<CharT, Traits> rhs) /*noexcept*/ {
    return lhs.equal(rhs);
}

// compare objects convertible to str_view for equality
template<class CharT, class Traits, class StrT,
class = std::enable_if<std::is_convertible<StrT, str_view<CharT, Traits>>::value>::type>
/*constexpr*/ bool operator==(StrT&& lhs, const str_view<CharT, Traits> rhs) {
    return rhs.equal(std::forward<StrT>(lhs));
}

template<class CharT, class Traits, class StrT,
class = std::enable_if<std::is_convertible<StrT, str_view<CharT, Traits>>::value>::type>
/*constexpr*/ bool operator==(const str_view<CharT, Traits> lhs, StrT&& rhs) {
    return lhs.equal(std::forward<StrT>(rhs));
}


#endif // STR_VIEW_H
