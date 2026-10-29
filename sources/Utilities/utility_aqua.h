#pragma once
#include <memory>

namespace utility_aqua
{
    template<typename W>
    struct aqua_opt
    {
    public:
        // 1. Конструктор по умолчанию
        aqua_opt() = default;

        // 2. Конструктор копирования (Глубокая копия)
        aqua_opt(const aqua_opt& other) {
            if (other.val) {
                val = std::make_unique<W>(*other.val);
            }
        }

        // 3. Оператор присваивания копированием
        aqua_opt& operator=(const aqua_opt& other) {
            if (this != &other) {
                if (other.val) {
                    if (val)
                        *val = *other.val;
                    else
                        val = std::make_unique<W>(*other.val);
                }
                else {
                    reset();
                }
            }
            return *this;
        }

        // 4. Конструктор перемещения (для оптимизации)
        aqua_opt(aqua_opt&& other) noexcept = default;

        // 5. Оператор присваивания перемещением
        aqua_opt& operator=(aqua_opt&& other) noexcept = default;

        // Деструктор теперь не нужен, unique_ptr все сделает сам

        W* emplace() {
            if (val)
                (*val) = W();
            else
                val = std::make_unique<W>();
            return val.get();
        }

        const W value() const {
            return (val ? *val : W());
        }

        const W value_or(const W or_passed) const {
            return (val ? *val : or_passed);
        }

        operator W* () {
            return val.get();
        }

        W* operator->() {
            return val.get();
        }

        const W* operator->() const {
            return val.get();
        }

        operator bool() const {
            return static_cast<bool>(val);
        }

        const W& operator=(const W& passed_val) {
            if (val)
                (*val) = passed_val;
            else
                val = std::make_unique<W>(passed_val);
            return *val;
        }

        void reset() {
            val.reset();
        }

    private:
        std::unique_ptr<W> val;
    };
};
