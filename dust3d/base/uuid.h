/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef DUST3D_BASE_UUID_H_
#define DUST3D_BASE_UUID_H_

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>
#include <string>

namespace dust3d
{

class Uuid
{
public:
    Uuid()
    {
        m_uuid = boost::uuids::nil_uuid();
    }
    
    Uuid(const boost::uuids::uuid &uuid) :
        m_uuid(uuid)
    {
    }
    
    Uuid(const std::string &string)
    {
        if (sizeof("{hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh}") - 1 != string.length() ||
                '{' != string[0]) {
            m_uuid = boost::uuids::nil_uuid();
            return;
        }
        m_uuid = boost::lexical_cast<boost::uuids::uuid>(
            string.substr(1, sizeof("hhhhhhhh-hhhh-hhhh-hhhh-hhhhhhhhhhhh") - 1));
    }
    
    bool isNull() const
    {
        return m_uuid.is_nil();
    }
    
    std::string toString() const
    {
        return "{" + to_string(m_uuid) + "}";
    }
    
    static Uuid createUuid()
    {
        return Uuid(boost::uuids::random_generator()());
    }
private:
    friend std::string to_string(const Uuid &uuid);
    friend struct std::hash<Uuid>;
    friend bool operator==(const Uuid &lhs, const Uuid &rhs);
    friend bool operator!=(const Uuid &lhs, const Uuid &rhs);
    friend bool operator<(const Uuid &lhs, const Uuid &rhs);
    boost::uuids::uuid m_uuid;
};

inline std::string to_string(const Uuid &uuid)
{
    return "{" + to_string(uuid.m_uuid) + "}";
}

inline bool operator==(const Uuid &lhs, const Uuid &rhs)
{
    return lhs.m_uuid == rhs.m_uuid;
}

inline bool operator!=(const Uuid &lhs, const Uuid &rhs)
{
    return lhs.m_uuid != rhs.m_uuid;
}

inline bool operator<(const Uuid &lhs, const Uuid &rhs)
{
    return lhs.m_uuid < rhs.m_uuid;
}
    
}

namespace std
{
    
template<>
struct hash<dust3d::Uuid>
{
    size_t operator()(const dust3d::Uuid& uuid) const
    {
        return boost::hash<boost::uuids::uuid>()(uuid.m_uuid);
    }
};

}

#endif
