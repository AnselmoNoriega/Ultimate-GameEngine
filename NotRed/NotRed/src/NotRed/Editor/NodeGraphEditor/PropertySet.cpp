#include <nrpch.h>

#include "PropertySet.h"

#include "yaml-cpp/yaml.h"
#include "NotRed/Util/YAMLSerializationHelpers.h"
#include "NotRed/Util/SerializationMacros.h"

namespace NR::Utils
{
    PropertySet::PropertySet() = default;
    PropertySet::~PropertySet() = default;
    PropertySet::PropertySet(PropertySet&&) = default;
    PropertySet& PropertySet::operator= (PropertySet&&) = default;

    PropertySet::PropertySet(const PropertySet& other) : dictionary(other.dictionary)
    {
        if (other.properties != nullptr)
            properties = std::make_unique<std::vector<Property>>(*other.properties);
    }

    PropertySet& PropertySet::operator= (const PropertySet& other)
    {
        if (other.properties != nullptr)
            properties = std::make_unique<std::vector<Property>>(*other.properties);

        dictionary = other.dictionary;
        return *this;
    }

    static std::string GetTypeName(const choc::value::Value& v)
    {
        bool isArray = v.isArray();
        choc::value::Type type = isArray ? v.getType().getElementType() : v.getType();

        if (type.isFloat())  return "Float";
        else if (type.isInt())    return "Int";
        else if (type.isBool())   return "Bool";
        else if (type.isString()) return "String";
        else if (type.isObject())
        {
            if (isArray && v[0].hasObjectMember("TypeName"))
                return v[0]["TypeName"].get<std::string>();
            else if (v.hasObjectMember("TypeName"))
                return v["TypeName"].get<std::string>();
        }

        else return "invalid";
    }

    static std::string PropertyToString(const soul::StringDictionary& stringDictionary, const PropertySet::Property& prop)
    {
        auto desc = choc::text::addDoubleQuotes(prop.name) + ": "
            + "{"
            + choc::text::addDoubleQuotes("Type") + ": " + choc::text::addDoubleQuotes(std::string(GetTypeName(prop.value))) + ", "
            + choc::text::addDoubleQuotes("Value") + ": ";

        return desc + choc::json::toString(prop.value) + "}";

        //const auto& type = prop.value.getType();

        ////assert(type.isPrimitive() || type.isString());

        //if (type.isString())
        //    return desc + choc::json::getEscapedQuotedString(prop.value.getString());

        //if (type.isFloat())     return desc + choc::json::doubleToString(prop.value.getFloat64());
        //if (type.isInt())   return desc + std::to_string(prop.value.getInt64());

        //return desc + prop.value.getDescription();
    }

    static std::string PropertySetToString(const soul::StringDictionary& stringDictionary, const std::vector<PropertySet::Property>* properties)
    {
        if (properties == nullptr || properties->empty())
            return {};

        auto content = soul::joinStrings(*properties, ", ", [&](auto& p) { return PropertyToString(stringDictionary, p); });

        return "{ " + content + " }";
    }

    bool PropertySet::IsEmpty() const { return Size() == 0; }
    size_t PropertySet::Size() const { return properties == nullptr ? 0 : properties->size(); }

    choc::value::Value PropertySet::GetValue(const std::string& name) const
    {
        NR_CORE_ASSERT(!name.empty());
        return GetValue(name, {});
    }

    choc::value::Value PropertySet::GetValue(const std::string& name, const choc::value::Value& defaultReturnValue) const
    {
        NR_CORE_ASSERT(!name.empty());

        if (properties != nullptr)
            for (auto& p : *properties)
                if (p.name == name)
                    return p.value;

        return defaultReturnValue;
    }

    bool PropertySet::HasValue(const std::string& name) const
    {
        NR_CORE_ASSERT(!name.empty());

        if (properties != nullptr)
            for (auto& p : *properties)
                if (p.name == name)
                    return true;

        return false;
    }

    bool PropertySet::GetBool(const std::string& name, bool defaultValue) const
    {
        auto v = GetValue(name);
        return !v.isVoid() ? v.getView().get<bool>() : defaultValue;
    }

    float PropertySet::GetFloat(const std::string& name, float defaultValue) const
    {
        auto v = GetValue(name);
        return v.getType().isFloat() || v.getType().isInt() ? v.getView().get<float>() : defaultValue;
    }

    double PropertySet::GetDouble(const std::string& name, double defaultValue) const
    {
        auto v = GetValue(name);
        return v.getType().isFloat() || v.getType().isInt() ? v.getView().get<double>() : defaultValue;
    }

    int32_t PropertySet::GetInt(const std::string& name, int32_t defaultValue) const
    {
        auto v = GetValue(name);
        return v.getType().isFloat() || v.getType().isInt() ? v.getView().get<int32_t>() : defaultValue;
    }

    int64_t PropertySet::GetInt64(const std::string& name, int64_t defaultValue) const
    {
        auto v = GetValue(name);
        return v.getType().isFloat() || v.getType().isInt() ? v.getView().get<int64_t>() : defaultValue;
    }

    std::string PropertySet::GetString(const std::string& name, const std::string& defaultValue) const
    {
        auto v = GetValue(name);

        if (!v.isVoid())
        {
            return std::string(v.getView().get<std::string>());

            /*struct UnquotedPrinter : public soul::ValuePrinter
            {
                std::ostringstream out;

                void print(std::string_view s) override { out << s; }
                void printStringLiteral(soul::StringDictionary::Handle h) override { print(dictionary->getStringForHandle(h)); }
            };

            UnquotedPrinter p;
            p.dictionary = std::addressof(dictionary);
            v.print(p);
            return p.out.str();*/
        }

        return defaultValue;
    }

    //static void replaceStringLiterals(choc::value::Value & v, soul::SubElementPath path, const soul::StringDictionary & sourceDictionary, soul::StringDictionary & destDictionary)
    //{
    //    auto value = v.getSubElement(path);
    //
    //    if (value.getType().isStringLiteral())
    //    {
    //        auto s = sourceDictionary.getStringForHandle(value.getStringLiteral());
    //        v.modifySubElementInPlace(path, choc::value::Value::createStringLiteral(destDictionary.getHandleForString(s)));
    //    }
    //    else if (value.getType().isFixedSizeArray())
    //    {
    //        for (size_t i = 0; i < value.getType().getArraySize(); ++i)
    //            replaceStringLiterals(v, path + i, sourceDictionary, destDictionary);
    //    }
    //    else if (value.getType().isStruct())
    //    {
    //        for (size_t i = 0; i < value.getType().getStructRef().getNumMembers(); ++i)
    //            replaceStringLiterals(v, path + i, sourceDictionary, destDictionary);
    //    }
    //}

    void PropertySet::SetInternal(const std::string& name, choc::value::Value newValue)
    {
        NR_CORE_ASSERT(!name.empty());

        if (properties == nullptr)
        {
            properties = std::make_unique<std::vector<Property>>();
        }
        else
        {
            for (auto& p : *properties)
            {
                if (p.name == name)
                {
                    p.value = std::move(newValue);
                    return;
                }
            }
        }

        properties->push_back({ name, std::move(newValue) });
    }

    void PropertySet::Set(const std::string& name, choc::value::Value newValue)
    {
        //replaceStringLiterals(newValue, {}, sourceDictionary, dictionary);
        SetInternal(name, std::move(newValue));
    }

    void PropertySet::Set(const std::string& name, int32_t value) { SetInternal(name, choc::value::createInt32(value)); }
    void PropertySet::Set(const std::string& name, int64_t value) { SetInternal(name, choc::value::createInt64(value)); }
    void PropertySet::Set(const std::string& name, float value) { SetInternal(name, choc::value::Value(value)); }
    void PropertySet::Set(const std::string& name, double value) { SetInternal(name, choc::value::Value(value)); }
    void PropertySet::Set(const std::string& name, bool value) { SetInternal(name, choc::value::Value(value)); }
    void PropertySet::Set(const std::string& name, const char* value) { Set(name, std::string(value)); }
    void PropertySet::Set(const std::string& name, const std::string& value) { SetInternal(name, choc::value::createString(value)); }

    void PropertySet::Set(const std::string& name, const choc::value::ValueView& value)
    {
        if (value.isInt32())    return Set(std::string(name), value.getInt32());
        if (value.isInt64())    return Set(std::string(name), value.getInt64());
        if (value.isFloat32())  return Set(std::string(name), value.getFloat32());
        if (value.isFloat64())  return Set(std::string(name), value.getFloat64());
        if (value.isBool())     return Set(std::string(name), value.getBool());
        if (value.isString())   return Set(std::string(name), std::string(value.getString()));

        return Set(name, choc::value::Value(value));

        NR_CORE_ASSERT(false); // other types not currently handled..
    }

    void PropertySet::Remove(const std::string& name)
    {
        NR_CORE_ASSERT(!name.empty());

        if (properties != nullptr)
            soul::removeIf(*properties, [&](const Property& p) { return p.name == name; });
    }

    std::vector<std::string> PropertySet::GetNames() const
    {
        std::vector<std::string> result;

        if (properties != nullptr)
        {
            result.reserve(properties->size());

            for (auto& p : *properties)
                result.push_back(p.name);
        }

        return result;
    }

    const soul::StringDictionary& PropertySet::GetDictionary() const { return dictionary; }

    //choc::value::Value PropertySet::toExternalValue() const
    //{
    //    auto o = choc::value::createObject("PropertySet");
    //
    //    if (properties != nullptr)
    //        for (auto& p : *properties)
    //            o.addMember(p.name, p.value.toExternalValue(ConstantTable(), dictionary));
    //
    //    return o;
    //}


    template<typename T>
    void ParseValueOrArray(PropertySet& ps, std::string_view name, const choc::value::ValueView& value)
    {
        if (value["Value"].isArray())
        {
            //auto arrayTypeFloat = choc::value::Type::createArray(choc::value::Type::createFloat32(), value["Value"].size());

            choc::value::Value valueArray = choc::value::createArray(value["Value"].size(), [&value](uint32_t i)
                { return value["Value"][i].get<T>(); });

            ps.Set(std::string(name), std::move(valueArray));
        }
        else
            ps.Set(std::string(name), choc::value::Value(value["Value"].get<T>()));
    }

    static choc::value::Value CreateAssetRefObject(uint64_t handle = 0)
    {
        return choc::value::createObject("AssetHandle",
            "TypeName", "AssetHandle",
            "Value", std::to_string(handle));
    }

    choc::value::Value ParseCustomValueType(const choc::value::ValueView& value)
    {
        if (value["TypeName"].isVoid())
        {
            NR_CORE_ASSERT(false, "Failed to deserialize custom value type, missing \"Type\" property.");
            return {};
        }

        choc::value::Value customObject = choc::value::createObject(value["TypeName"].get<std::string>());
        if (value.isObject())
        {
            for (uint32_t i = 0; i < value.size(); i++)
            {
                choc::value::MemberNameAndValue nameValue = value.getObjectMemberAt(i);
                customObject.addMember(nameValue.name, nameValue.value);
            }
        }
        else
        {
            NR_CORE_ASSERT(false, "Failed to load custom value type. It must be serialized as object.")
        }

        return customObject;
    };

    void ParseCustomObject(PropertySet& ps, std::string_view name, const choc::value::ValueView& value)
    {
        if (value["Value"].isArray())
        {
            choc::value::ValueView inArray = value["Value"];
            std::string type = value["Type"].get<std::string>();
            const uint32_t inArraySize = inArray.size();

            NR_CORE_ASSERT(type == "AssetHandle", "The only custom type is supported for now is \"AssetHandle\".");
            NR_CORE_ASSERT(inArraySize > 0);

            choc::value::Value valueArray = choc::value::createEmptyArray();

            for (int i = 0; i < inArraySize; ++i)
            {
                choc::value::Value vobj = CreateAssetRefObject();
                vobj["Value"].set(inArray[i]["Value"].get<std::string>());
                valueArray.addArrayElement(vobj);
            }
            NR_CORE_ASSERT(valueArray.size() == inArraySize)

                ps.Set(std::string(name), valueArray);

            auto et = ps.GetValue(std::string(name)).getType().getElementType();
        }
        else
            ps.Set(std::string(name), ParseCustomValueType(value["Value"]));
    }

    PropertySet PropertySet::FromExternalValue(const choc::value::ValueView& v)
    {
        PropertySet a;

        //if (v.isObjectWithClassName(objectClassName /*"PropertySet"*/))
        {
            v.visitObjectMembers([&](std::string_view name, const choc::value::ValueView& value)
                {
                    // Need to store type as a property of each value object
                    // because .json can't distinguish between Int and Float if the value is 0.0,
                    // it just knows type "number"
                    if (value.isObject() && value.hasObjectMember("Type"))
                    {
                        std::string type(value["Type"].getString());

                        if (type == "Float")
                            ParseValueOrArray<float>(a, name, value);
                        else if (type == "Int")
                            ParseValueOrArray<int32_t>(a, name, value);
                        else if (type == "Bool")
                            ParseValueOrArray<bool>(a, name, value);
                        else
                            ParseCustomObject(a, name, value);
                    }
                    else
                        a.Set(std::string(name), value);
                });
        }

        return a;
    }

    std::string PropertySet::ToJSON() const { return PropertySetToString(dictionary, properties.get()); }

#if 0
    template<typename T>
    T ParseValueOrArrayToYAML(PropertySet& ps, std::string_view name, const choc::value::ValueView& value)
    {
        if (value["Value"].isArray())
        {
            auto arrayTypeFloat = choc::value::Type::createArray(choc::value::Type::createFloat32(), value["Value"].size());

            choc::value::Value valueArray = choc::value::createArray(value["Value"].size(), [&value](uint32_t i)
                { return value["Value"][i].get<T>(); });

            ps.Set(std::string(name), std::move(valueArray));
        }
        else
            return value["Value"].get<T>());
    }

    std::string PropertySet::ToYAML() const
    {
        YAML::Emitter out;
        out << YAML::Key << "Properties" << YAML::Value;
        out << YAML::BeginSeq;


        for (const auto& prop : *properties.get())
        {
            out << YAML::BeginMap; // properties

            prop.value.getType()

                out << YAML::EndMap; // properties
        }

        out << YAML::EndSeq; // Properties


        return std::string();
    }

#endif

} // namespace Hazel::Utils