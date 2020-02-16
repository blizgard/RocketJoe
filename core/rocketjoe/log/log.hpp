#pragma once

#include <string>
#include <iostream>
#include <chrono>
#include <mutex>
#include <spdlog/async_logger.h>

namespace rocketjoe {

    class log_t final {
    public:

        log_t() = default;

        log_t(std::shared_ptr<spdlog::async_logger> logger);

        ~log_t() = default;

        auto clone() noexcept -> log_t;

        auto context(std::shared_ptr<spdlog::async_logger> logger) noexcept -> void;

        template<typename MSGBuilder>
        auto trace(MSGBuilder &&msg_builder) noexcept -> void {
            logger_->trace(std::forward<MSGBuilder>(msg_builder));
        }

        template<typename MSGBuilder>
        auto info(MSGBuilder &&msg_builder) noexcept -> void {
            logger_->info(std::forward<MSGBuilder>(msg_builder));
        }

        template<typename MSGBuilder>
        auto warn(MSGBuilder &&msg_builder) noexcept -> void {
            logger_->warn(std::forward<MSGBuilder>(msg_builder));
        }

        template<typename MSGBuilder>
        auto error(MSGBuilder &&msg_builder) noexcept -> void {
            logger_->error(std::forward<MSGBuilder>(msg_builder));
        }

        template<typename MSGBuilder>
        auto debug(MSGBuilder &&msg_builder) noexcept -> void {
            logger_->debug(std::forward<MSGBuilder>(msg_builder));
        }

        template<typename MSGBuilder>
        auto critical(MSGBuilder &&msg_builder) noexcept -> void {
            logger_->critical(std::forward<MSGBuilder>(msg_builder));
        }

    private:
        std::shared_ptr<spdlog::async_logger> logger_;
    };
}