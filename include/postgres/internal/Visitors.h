#pragma once

#include <chrono>
#include <string>
#include <type_traits>
#include <vector>
#include <optional>

#include <postgres/Oid.h>
#include <postgres/Enum.h>

namespace postgres::internal {

struct FieldsCollector {
    template <typename T>
    void accept(char const* const name) {
        if (!res.empty()) {
            res += ",";
        }
        res += name;
    }

    std::string res;
};

struct TypedFieldsCollector {
    template <typename T>
    void accept(char const* const name) {
        if (!res.empty()) {
            res += ",";
        }
        res += name;
        res += " ";
        res += type(static_cast<T*>(nullptr));
    }

    std::string res;

private:
    template <typename T>
    std::enable_if_t<std::is_arithmetic_v<T>, char const*> type(T*) {
        if (std::is_same_v<T, bool>) {
            return "BOOL";
        }

        auto constexpr SIZE = sizeof(T);
        if (std::is_floating_point_v<T>) {
            if (SIZE <= 4) {
                return "REAL";
            }
            return "DOUBLE PRECISION";
        }

        if (std::is_signed_v<T>) {
            if (SIZE <= 2) {
                return "SMALLINT";
            }
            if (SIZE <= 4) {
                return "INT";
            }
            return "BIGINT";
        }

        if (SIZE <= 2) {
            return "SMALLSERIAL";
        }
        if (SIZE <= 4) {
            return "SERIAL";
        }
        return "BIGSERIAL";
    }

    char const* type(std::string*) {
        return "TEXT";
    }

    char const* type(std::vector<std::string>*) {
        return "TEXT[]";
    }

    char const* type(std::chrono::system_clock::time_point*) {
        return "TIMESTAMP";
    }

    template <typename T>
    std::enable_if_t<std::is_base_of_v<postgres::Enum, T>, char const* > type(T*) {
        return T::name;
    }

    template <typename T>
    std::enable_if_t<std::is_base_of_v<postgres::Enum, T>, char const* > type(std::vector<T>*) {
        static auto const cache = std::string{T::name} + "[]";
        return cache.c_str();
    }
};


struct TypesCollector {
   template <typename T> void accept(char const *const) {
    types.push_back(oid_of(static_cast<T *>(nullptr)));
  }

  std::vector<Oid> types;

private:
    template <typename T>
    std::enable_if_t<std::is_arithmetic_v<T>, Oid> oid_of(T*) {
        if (std::is_same_v<T, bool>) {
            return BOOLOID;
        }

        auto constexpr SIZE = sizeof(T);
        if (std::is_floating_point_v<T>) {
            if (SIZE <= 4) {
                return FLOAT4OID;
            }
            return FLOAT8OID;
        }

        if (std::is_signed_v<T>) {
            if (SIZE <= 2) {
                return INT2OID;
            }
            if (SIZE <= 4) {
                return INT4OID;
            }
            return INT8OID;
        }

        if (SIZE <= 2) {
            return INT2OID;
        }
        if (SIZE <= 4) {
            return INT4OID;
        }
        return INT8OID;
    }

    Oid oid_of(std::string*) {
        return TEXTOID;
    }

    Oid oid_of(std::vector<std::string>*) {
        return TEXTARRAYOID;
    }

    Oid oid_of(std::chrono::system_clock::time_point*) {
        return TIMESTAMPOID;
    }

    template<typename T> Oid oid_of(std::optional<T>*) {
        return oid_of(static_cast<T *>(nullptr));
    }

    template <typename T>
    std::enable_if_t<std::is_base_of_v<postgres::Enum, T>,Oid> oid_of(T*) {
        return UNKNOWNOID;
    }

    template <typename T>
    std::enable_if_t<std::is_base_of_v<postgres::Enum, T>,Oid> oid_of(std::vector<T>*) {
        return UNKNOWNOID;
    }

};


struct PlaceholdersCollector {
    template <typename T>
    void accept(char const* const) {
        res += res.empty() ? "$" : ",$";
        res += std::to_string(++idx);
    }

    int         idx = 0;
    std::string res{};
};


struct CastedPlaceholdersCollector {
    template <typename T>
    void accept(char const* const) {
        res += res.empty() ? "$" : ",$";
        res += std::to_string(++idx);

        auto [casting_type, is_array]  = needs_casting(static_cast<T*>(nullptr));
        if(casting_type != nullptr){
            res+="::";
            res+= casting_type;

            if(is_array){
                res += "[]";
            }
        }

    }

    int         idx = 0;
    std::string res{};

private:
    template <typename T>
    std::pair<const char*, bool> needs_casting(T*) {
        if constexpr(std::is_base_of<postgres::Enum, T>::value){
            return {T::name, false};
        }

        return {nullptr, false};
    }

    template <typename T>
    std::enable_if_t<std::is_base_of_v<postgres::Enum, T>, std::pair<const char*, bool>> needs_casting(std::vector<T>**) {
        return {T::name, true};
    }

};



struct AssignmentsCollector {
    template <typename T>
    void accept(char const* const name) {
        if (!res.empty()) {
            res += ",";
        }
        res += name;
        res += "=$";
        res += std::to_string(++idx);
    }

    int         idx = 0;
    std::string res{};
};

}  // namespace postgres::internal
