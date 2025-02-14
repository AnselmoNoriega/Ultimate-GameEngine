#pragma once
#include "NotRed/Vendor/SOUL/SOUL.h"

namespace NR::Utils
{
    //==============================================================================
    /** A set of named properties. The value of each item is a Value object. */
    struct PropertySet
    {
        PropertySet();
        ~PropertySet();
        PropertySet(const PropertySet&);
        PropertySet(PropertySet&&);
        PropertySet& operator= (const PropertySet&);
        PropertySet& operator= (PropertySet&&);

        struct Property
        {
            std::string name;
            choc::value::Value value;
        };

        bool IsEmpty() const;
        size_t Size() const;

        choc::value::Value GetValue(const std::string& name) const;
        choc::value::Value GetValue(const std::string& name, const choc::value::Value& defaultReturnValue) const;
        bool HasValue(const std::string& name) const;

        bool GetBool(const std::string& name, bool defaultValue = false) const;
        float GetFloat(const std::string& name, float defaultValue = 0.0f) const;
        double GetDouble(const std::string& name, double defaultValue = 0.0) const;
        int32_t GetInt(const std::string& name, int32_t defaultValue = 0) const;
        int64_t GetInt64(const std::string& name, int64_t defaultValue = 0) const;
        std::string GetString(const std::string& name, const std::string& defaultValue = {}) const;

        void Set(const std::string& name, int32_t value);
        void Set(const std::string& name, int64_t value);
        void Set(const std::string& name, float value);
        void Set(const std::string& name, double value);
        void Set(const std::string& name, bool value);
        void Set(const std::string& name, const char* value);
        void Set(const std::string& name, const std::string& value);
        void Set(const std::string& name, choc::value::Value newValue);
        void Set(const std::string& name, const choc::value::ValueView& value);

        void Remove(const std::string& name);

        std::vector<std::string> GetNames() const;
        const soul::StringDictionary& GetDictionary() const;

        static PropertySet FromExternalValue(const choc::value::ValueView& v);

        std::string ToJSON() const;

    private:
        std::unique_ptr<std::vector<Property>> properties;
        soul::StringDictionary dictionary;

        void Set(const std::string& name, const void* value) = delete;
        void SetInternal(const std::string& name, choc::value::Value newValue);
    };

} 