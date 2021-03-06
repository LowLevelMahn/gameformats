#include "cmp.h"

#include <sstream>
#include <stdexcept>

using namespace cmp;

const int cmp::MaxMaterials   = 11;
const int cmp::MaxMatrices    = 37;
const int cmp::MaxDemolitions = 23;

Element::Element(Version version)
{
	switch (version) {
		case Version109:
		case Version114:
		case Version115:
			break;
		default:
			std::ostringstream msg;
			msg << "Unknown CMP version. Expected one of " << Version109 << "/" << Version114 << "/" << Version115 << ", got " << version << ".";
			throw std::runtime_error(msg.str());
	}

	this->version = version;
}

Node::Node(Version version, Type type) : Element(version)
{
	switch (type) {
		case Root:
		case Transform:
		case Mesh:
		case Axis:
		case Light:
		case Smoke:
		case MultiMesh:
			break;
		default:
			std::ostringstream msg;
			msg << "Unknown node type. Expected " << Root << " - " << MultiMesh << ", got " << type << ".";
			throw std::runtime_error(msg.str());
	}

	this->type = type;
}

Node::~Node()
{
}

void Node::read(std::ifstream& ifs)
{
	parse(ifs, name);
}

Node* Node::readNode(std::ifstream& ifs, Version version)
{
	Node* node;

	Type type;
	parse(ifs, type);

	switch (type) {
		case Root:
			throw std::runtime_error("Unexpected root node. Use RootNode::readFile() to parse file from the beginning.");
		case Transform:
			node = new TransformNode(version);
			break;
		case Mesh:
		case MultiMesh:
			node = new MeshNode(version, type);
			break;
		case Axis:
			node = new AxisNode(version);
			break;
		case Light:
			node = new LightNode(version);
			break;
		case Smoke:
			node = new SmokeNode(version);
			break;
		default:
			std::ostringstream msg;
			msg << "Unknown node type. Expected " << Transform << " - " << MultiMesh << ", got " << type << ".";
			throw std::runtime_error(msg.str());
	}

	node->read(ifs);

	return node;
}

GroupNode::~GroupNode()
{
	for (Node* child : children) {
		delete child;
	}
}

void GroupNode::read(std::ifstream& ifs)
{
	uint32_t childCount;
	parse(ifs, childCount);

	for (uint32_t i = 0; i < childCount; i++) {
		children.push_back(readNode(ifs, version));
	}
}

void GroupNode::findMeshes(MeshList* meshList)
{
	for (Node* child : children) {
		child->findMeshes(meshList);
	}
}

RootNode::RootNode(Version version) : GroupNode(version, Root)
{
	rootEntries = 0;
}

RootNode::~RootNode()
{
	if (rootEntries) {
		delete[] rootEntries;
	}
}

RootNode* RootNode::readFile(std::ifstream& ifs)
{
	Type type;
	parse(ifs, type);

	if (type != Root) {
		std::ostringstream msg;
		msg << "Unexpected root node type. Expected " << Root << ", got " << type << ".";
		throw std::runtime_error(msg.str());
	}

	Version version;
	parse(ifs, version);

	RootNode* root = new RootNode(version);

	root->read(ifs);

	root->resolveReferences();

	return root;
}

void RootNode::read(std::ifstream& ifs)
{
	Node::read(ifs);

	parse(ifs, unknown0);
	parse(ifs, aabb);
	parse(ifs, unknown1);
	parse(ifs, unknown2);
	parse(ifs, unknown3);

	if (version >= Version114) {
		parse(ifs, path);
	}

	parse(ifs, transformation);
	parse(ifs, unknown4);
	parse(ifs, unknown5);
	parse(ifs, aabb2);
	parse(ifs, rootEntryCount);
	parse(ifs, unknown6);
	parse(ifs, unknown7);
	parse(ifs, unknown8);
	parse(ifs, unknown9);

	rootEntries = new RootEntry[rootEntryCount];
	ifs.read(reinterpret_cast<char*>(rootEntries), sizeof(RootEntry) * rootEntryCount);

	parse(ifs, matrixCount);

	GroupNode::read(ifs);
}

void RootNode::resolveReferences()
{
	std::vector<MeshData*> meshes;
	findMeshes(&meshes);

	for (MeshData* empty : meshes) {
		if (empty->length == 0) {
			for (MeshData* mesh : meshes) {
				if (mesh != empty && mesh->length > 0 && mesh->name == empty->name) {
					empty->reference = mesh;
					break;
				}
			}
		}
	}
}

void TransformNode::read(std::ifstream& ifs)
{
	Node::read(ifs);

	parse(ifs, transformation);
	parse(ifs, matrixId);
	parse(ifs, aabb);

	GroupNode::read(ifs);
}

void AxisNode::read(std::ifstream& ifs)
{
	Node::read(ifs);
}

void LightNode::read(std::ifstream& ifs)
{
	Node::read(ifs);

	parse(ifs, lightType);
	parse(ifs, isTogglable);
	parse(ifs, unknown0);
	parse(ifs, brightness);
	parse(ifs, unknown1);
	parse(ifs, size);
	parse(ifs, unknown2);
	parse(ifs, color);

	if (version >= Version114) {
		parse(ifs, unknown3);
		parse(ifs, unknown4);
	}
}

void SmokeNode::read(std::ifstream& ifs)
{
	Node::read(ifs);
	parse(ifs, unknown0);
}

MeshData::MeshData(Version version) : Element(version)
{
	indices = 0;
	vertices = 0;
	numberPlateVertices = 0;
	reference = 0;
}

MeshData::~MeshData()
{
	if (indices) {
		delete[] indices;
	}

	if (vertices) {
		delete[] vertices;
	}

	for (Primitive* primitive : primitives) {
		delete primitive;
	}

	for (Material* material : materials) {
		delete material;
	}

	if (numberPlateVertices) {
		delete numberPlateVertices;
	}
}

void MeshData::read(std::ifstream& ifs)
{
	parse(ifs, name);
	parse(ifs, length);

	if (!length) {
		return;
	}

	int meshStartOffset = (int)ifs.tellg();

	parse(ifs, unknown0);
	parse(ifs, aabb);
	parse(ifs, vertexCount1);
	parse(ifs, indexCount);
	parse(ifs, unknown1);
	parse(ifs, unknown2);
	parse(ifs, unknown3);

	if (version >= Version115) {
		parse(ifs, color);
	}
	else {
		parse(ifs, color.a);
	}

	parse(ifs, path);

	parse(ifs, hasIndices);
	if (hasIndices) {
		parse(ifs, unknown4);
		parse(ifs, indicesLength);

		indices = new uint16_t[indexCount];
		ifs.read(reinterpret_cast<char*>(indices), indicesLength);
	}

	parse(ifs, unknown5);
	parse(ifs, unknown6);
	parse(ifs, unknown7);

	parse(ifs, vertexCount2);

	if (vertexCount1 != vertexCount2) {
		std::ostringstream msg;
		msg << "vertexCount1 (" << vertexCount1 << ") != vertexCount2 (" << vertexCount2 << ") in mesh \"" << name << "\".";
		throw std::runtime_error(msg.str());
	}

	parse(ifs, vertexStride);

	if (vertexStride != sizeof(Vertex)) {
		std::ostringstream msg;
		msg << "vertexStride (" << vertexStride << ") != sizeof(Vertex) (" << sizeof(Vertex) << ") in mesh \"" << name << "\".";
		throw std::runtime_error(msg.str());
	}

	parse(ifs, verticesLength);
	parse(ifs, unknown8);

	vertices = new Vertex[vertexCount2];
	ifs.read(reinterpret_cast<char*>(vertices), verticesLength);

	parse(ifs, primitiveAndMaterialCount);

	for (unsigned i = 0; i < primitiveAndMaterialCount / 2; i++) {
		Primitive::Type type;
		parse(ifs, type);

		Primitive* primitive = 0;

		switch (type) {
			case Primitive::TriangleList:
			{
				TriangleList* list = new TriangleList();
				primitive = list;
				parse(ifs, list->minIndex);
				parse(ifs, list->vertexCount);
				break;
			}
			case Primitive::TriangleStrip:
			{
				TriangleStrip* strip = new TriangleStrip();
				primitive = strip;
				break;
			}
			default:
				std::ostringstream msg;
				msg << "Unknown primitive type " << (int)type << " in mesh \"" << name << "\".";
				throw std::runtime_error(msg.str());
		}

		parse(ifs, primitive->offset);
		parse(ifs, primitive->count);
		parse(ifs, primitive->unknown);
		primitives.push_back(primitive);
	}

	parse(ifs, materialCount);

	for (unsigned i = 0; i < materialCount; i++) {
		Material* material = new Material();
		parse(ifs, material->minIndex);
		parse(ifs, material->vertexCount);
		parse(ifs, material->offset);
		parse(ifs, material->count);
		parse(ifs, material->isTriangleStrip);
		parse(ifs, material->material);
		materials.push_back(material);
	}

	if (version >= Version115) {
		parse(ifs, hasNumberPlate);

		if (hasNumberPlate) {
			parse(ifs, numberPlateVertexCount);
			numberPlateVertices = new NumberPlateVertex[numberPlateVertexCount];
			ifs.read(reinterpret_cast<char*>(numberPlateVertices), sizeof(NumberPlateVertex) * numberPlateVertexCount);
		}
	}

	int unparsedLength = length - ((int)ifs.tellg() - meshStartOffset);

	if (unparsedLength < 0) {
		std::ostringstream msg;
		msg << "Read " << -unparsedLength << " bytes past expected length of " << length << " bytes in mesh \"" << name << "\".";
		throw std::runtime_error(msg.str());
	}
	else if (unparsedLength > 0) {
		std::ostringstream msg;
		msg << "Finished reading mesh \"" << name << "\" with " << unparsedLength << " unparsed bytes left.";
		throw std::runtime_error(msg.str());
	}
}

MeshNode::~MeshNode()
{
	for (MeshData* mesh : meshes) {
		delete mesh;
	}
}

void MeshNode::findMeshes(MeshList* meshList)
{
	for (cmp::MeshData* mesh : meshes) {
		meshList->push_back(mesh);
	}
}

void MeshNode::read(std::ifstream& ifs)
{
	Node::read(ifs);

	parse(ifs, unknown0);
	parse(ifs, loose);
	parse(ifs, drop);
	parse(ifs, unknown1);

	int maxMeshes = 2;
	if (type == MultiMesh) {
		maxMeshes = 3;
		parse(ifs, aabb);
	}

	for (int i = 0; i < maxMeshes; i++) {
		uint8_t meshFollows;
		parse(ifs, meshFollows);

		if (meshFollows) {
			MeshData* mesh = new MeshData(version);
			mesh->read(ifs);
			meshes.push_back(mesh);
		}
		else {
			break;
		}
	}
}

std::ostream& operator<<(std::ostream& lhs, cmp::Node::Type type)
{
	switch (type) {
		case cmp::Node::Root:      lhs << "Root";      break;
		case cmp::Node::Transform: lhs << "Transform"; break;
		case cmp::Node::Mesh:      lhs << "Mesh";      break;
		case cmp::Node::Axis:      lhs << "Axis";      break;
		case cmp::Node::Light:     lhs << "Light";     break;
		case cmp::Node::Smoke:     lhs << "Smoke";     break;
		case cmp::Node::MultiMesh: lhs << "MultiMesh"; break;
		default: lhs << "Unknown (" << (uint32_t)type << ")";
	}

	return lhs;
}

std::ostream& operator<<(std::ostream& lhs, cmp::Primitive::Type type)
{
	switch (type) {
		case cmp::Primitive::TriangleList:  lhs << "TriangleList";  break;
		case cmp::Primitive::TriangleStrip: lhs << "TriangleStrip"; break;
		default: lhs << "Unknown (" << (uint16_t)type << ")";
	}

	return lhs;
}

std::ostream& operator<<(std::ostream& lhs, cmp::LightNode::LightType type)
{
	switch (type) {
		case cmp::LightNode::HeadLight:    lhs << "HeadLight";    break;
		case cmp::LightNode::BackLight:    lhs << "BackLight";    break;
		case cmp::LightNode::BrakeLight:   lhs << "BrakeLight";   break;
		case cmp::LightNode::ReverseLight: lhs << "ReverseLight"; break;
		case cmp::LightNode::Siren:        lhs << "Siren";        break;
		case cmp::LightNode::SignalLeft:   lhs << "SignalLeft";   break;
		case cmp::LightNode::SignalRight:  lhs << "SignalRight";  break;
		case cmp::LightNode::HeadLightEnv: lhs << "HeadLightEnv"; break;
		case cmp::LightNode::SirenEnv:     lhs << "SirenEnv";     break;
		default: lhs << "Unknown (" << (uint32_t)type << ")";
	}

	return lhs;
}

std::ostream& operator<<(std::ostream& lhs, cmp::Color4b color)
{
	return lhs << "(R: " << (int)color.r << " G: " << (int)color.g << " B: " << (int)color.b << " A: " << (int)color.a << ")";
}
