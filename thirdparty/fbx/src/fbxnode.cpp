#include "fbxnode.h"

#include "fbxutil.h"
using std::string;
using std::cout;
using std::endl;
using std::ifstream;
using std::uint32_t;
using std::uint8_t;

namespace fbx {

FBXNode::FBXNode()
{
}

FBXNode::FBXNode(std::string name):name(name) {}

uint32_t FBXNode::read(std::ifstream &input, uint32_t start_offset)
{
    Reader reader(&input);
    uint32_t bytes = 0;

    uint32_t endOffset = reader.readUint32();
    uint32_t numProperties = reader.readUint32();
    uint32_t propertyListLength = reader.readUint32();
    uint8_t nameLength = reader.readUint8();
    name = reader.readString(nameLength);
    bytes += 13 + nameLength;

    //std::cout << "so: " << start_offset
    //          << "\tbytes: " << (endOffset == 0 ? 0 : (endOffset - start_offset))
    //          << "\tnumProp: " << numProperties
    //          << "\tpropListLen: " << propertyListLength
    //          << "\tnameLen: " << std::to_string(nameLength)
    //          << "\tname: " << name << "\n";

    for(uint32_t i = 0; i < numProperties; i++) {
        addProperty(FBXProperty(input));
    }
    bytes += propertyListLength;

    while(start_offset + bytes < endOffset) {
        FBXNode child;
        bytes += child.read(input, start_offset + bytes);
        addChild(std::move(child));
    }
    return bytes;
}

uint32_t FBXNode::write(std::ofstream &output, uint32_t start_offset)
{
    Writer writer(&output);

    if(isNull()) {
        //std::cout << "so: " << start_offset
        //          << "\tbytes: 0"
        //          << "\tnumProp: 0"
        //          << "\tpropListLen: 0"
        //          << "\tnameLen: 0"
        //          << "\tname: \n";
        for(int i = 0; i < 13; i++) writer.write((uint8_t) 0);
        return 13;
    }

    uint32_t propertyListLength = 0;
    for(auto prop : properties) propertyListLength += prop.getBytes();
    uint32_t bytes = 13 + name.length() + propertyListLength;
    for(auto child : children) bytes += child.getBytes();

    if(bytes != getBytes()) throw std::string("bytes != getBytes()");
    writer.write(start_offset + bytes); // endOffset
    writer.write((uint32_t) properties.size()); // numProperties
    writer.write(propertyListLength); // propertyListLength
    writer.write((uint8_t) name.length());
    writer.write(name);

    //std::cout << "so: " << start_offset
    //          << "\tbytes: " << bytes
    //          << "\tnumProp: " << properties.size()
    //          << "\tpropListLen: " << propertyListLength
    //          << "\tnameLen: " << name.length()
    //          << "\tname: " << name << "\n";

    bytes = 13 + name.length() + propertyListLength;

    for(auto prop : properties) prop.write(output);
    for(auto child : children) bytes += child.write(output,  start_offset + bytes);

    return bytes;
}

void FBXNode::print(std::string prefix)
{
    cout << prefix << "{ \"name\": \"" << name << "\"" << (properties.size() + children.size() > 0 ? ",\n" : "\n");
    if(properties.size() > 0) {
        cout << prefix << "  \"properties\": [\n";
        bool hasPrev = false;
        for(FBXProperty prop : properties) {
            if(hasPrev) cout << ",\n";
            cout << prefix << "    { \"type\": \"" << prop.getType() << "\", \"value\": " << prop.to_string() << " }";
            hasPrev = true;
        }
        cout << "\n";
        cout << prefix << "  ]" << (children.size() > 0 ? ",\n" : "\n");

    }

    if(children.size() > 0) {
        cout << prefix << "  \"children\": [\n";
        bool hasPrev = false;
        for(FBXNode node : children) {
            if(hasPrev) cout << ",\n";
            node.print(prefix+"    ");
            hasPrev = true;
        }
        cout << "\n";
        cout << prefix << "  ]\n";
    }

    cout << prefix << "}";

}

bool FBXNode::isNull()
{
    return children.size() == 0
            && properties.size() == 0
            && name.length() == 0;
}

// primitive values
void FBXNode::addProperty(int16_t v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(bool v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(int32_t v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(float v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(double v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(int64_t v) { addProperty(FBXProperty(v)); }
// arrays
void FBXNode::addProperty(const std::vector<bool> v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(const std::vector<int32_t> v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(const std::vector<float> v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(const std::vector<double> v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(const std::vector<int64_t> v) { addProperty(FBXProperty(v)); }
// raw / string
void FBXNode::addProperty(const std::vector<uint8_t> v, uint8_t type) { addProperty(FBXProperty(v, type)); }
void FBXNode::addProperty(const std::string v) { addProperty(FBXProperty(v)); }
void FBXNode::addProperty(const char *v) { addProperty(FBXProperty(v)); }

void FBXNode::addProperty(FBXProperty prop) { properties.push_back(prop); }


void FBXNode::addPropertyNode(const std::string name, int16_t v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, bool v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, int32_t v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, float v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, double v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, int64_t v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, const std::vector<bool> v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, const std::vector<int32_t> v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, const std::vector<float> v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, const std::vector<double> v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, const std::vector<int64_t> v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, const std::vector<uint8_t> v, uint8_t type) { FBXNode n(name); n.addProperty(v, type); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, const std::string v) { FBXNode n(name); n.addProperty(v); addChild(n); }
void FBXNode::addPropertyNode(const std::string name, const char *v) { FBXNode n(name); n.addProperty(v); addChild(n); }

void FBXNode::addChild(FBXNode child) { children.push_back(child); }

uint32_t FBXNode::getBytes() {
    uint32_t bytes = 13 + name.length();
    for(auto child : children) {
        bytes += child.getBytes();
    }
    for(auto prop : properties) {
        bytes += prop.getBytes();
    }
    return bytes;
}

const std::vector<FBXNode> FBXNode::getChildren()
{
    return children;
}

const std::string FBXNode::getName()
{
    return name;
}

} // namespace fbx
