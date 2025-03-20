#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

class noncopyable {
protected:
noncopyable() = default;
    ~noncopyable() = default;

    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

class noncopymoveable {
protected:
    noncopymoveable() = default;
    ~noncopymoveable() = default;

    noncopymoveable(const noncopymoveable&) = delete;
    noncopymoveable& operator=(const noncopymoveable&) = delete;

    noncopymoveable(noncopymoveable&&) = delete;
    noncopymoveable& operator=(noncopymoveable&&) = delete;
};
#endif // NONCOPYABLE_H