#pragma once

#include <optional>
#include <utility>

#include <QString>

template<typename T>
class Result final {
public:
    static Result success(T value)
    {
        return Result(std::move(value), QString{});
    }

    static Result failure(QString error)
    {
        return Result(std::nullopt, std::move(error));
    }

    bool isSuccess() const noexcept
    {
        return value_.has_value();
    }

    const T& value() const
    {
        return value_.value();
    }

    T takeValue()
    {
        return std::move(value_.value());
    }

    const QString& error() const noexcept
    {
        return error_;
    }

private:
    Result(T value, QString error)
        : value_(std::move(value)), error_(std::move(error))
    {
    }

    Result(std::optional<T> value, QString error)
        : value_(std::move(value)), error_(std::move(error))
    {
    }

    std::optional<T> value_;
    QString error_;
};

template<>
class Result<void> final {
public:
    static Result success()
    {
        return Result(true, QString{});
    }

    static Result failure(QString error)
    {
        return Result(false, std::move(error));
    }

    bool isSuccess() const noexcept
    {
        return success_;
    }

    const QString& error() const noexcept
    {
        return error_;
    }

private:
    Result(bool success, QString error)
        : success_(success), error_(std::move(error))
    {
    }

    bool success_ = false;
    QString error_;
};
