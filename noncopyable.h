#pragma once

/*
确保对象的唯一性，防止意外的拷贝导致资源管理问题
*/
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};