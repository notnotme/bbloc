#include "SpriteBuffer.h"

#include <glm/ext.hpp>

SpriteBuffer::SpriteBuffer(size_t size) :
mVertexBufferId(0),
mVertexArrayId(0),
mStorage(nullptr),
mSize(size),
mIndex(0) {
}

SpriteBuffer::~SpriteBuffer() {
}

void SpriteBuffer::initialize(const SpriteVertexAttribute& vertexattribute, Usage usage) {
    glGenBuffers(1, &mVertexBufferId);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, mSize * sizeof(SpriteVertex), nullptr, usage);

    glGenVertexArrays(1, &mVertexArrayId);
    glBindVertexArray(mVertexArrayId);
    glEnableVertexAttribArray(vertexattribute.position);
    glEnableVertexAttribArray(vertexattribute.size);
    glEnableVertexAttribArray(vertexattribute.texture);
    glEnableVertexAttribArray(vertexattribute.tint);
    glVertexAttribDivisor(vertexattribute.position, 1);
    glVertexAttribDivisor(vertexattribute.size, 1);
    glVertexAttribDivisor(vertexattribute.texture, 1);
    glVertexAttribDivisor(vertexattribute.tint, 1);

    glVertexAttribPointer(vertexattribute.position, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
        (void*) offsetof(SpriteVertex, position));

    glVertexAttribPointer(vertexattribute.size, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
        (void*) offsetof(SpriteVertex, size));

    glVertexAttribPointer(vertexattribute.texture, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
        (void*) offsetof(SpriteVertex, texture));

    glVertexAttribPointer(vertexattribute.tint, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SpriteVertex),
        (void*) offsetof(SpriteVertex, tint));
}

void SpriteBuffer::finalize() {
    glDeleteBuffers(1, &mVertexBufferId);
    glDeleteVertexArrays(1, &mVertexArrayId);
}

void SpriteBuffer::use() {
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferId);
    glBindVertexArray(mVertexArrayId);
}

void SpriteBuffer::map() {
    auto storage = glMapBufferRange(GL_ARRAY_BUFFER, 0, mSize * sizeof(SpriteVertex),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

    mIndex = 0;
    mStorage = static_cast<SpriteVertex*>(storage);
}

void SpriteBuffer::unmap() {
    glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, mIndex * sizeof(SpriteVertex));
    glUnmapBuffer(GL_ARRAY_BUFFER);
}

void SpriteBuffer::add(const SpriteVertex& vertex) {
    mStorage[mIndex] = vertex;
    ++mIndex;
}

void SpriteBuffer::update(size_t index, const SpriteVertex& vertex) {
    glBufferSubData(GL_ARRAY_BUFFER, index * sizeof(SpriteVertex), sizeof(SpriteVertex), &vertex);
}

void SpriteBuffer::draw() {
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mIndex);
}

void SpriteBuffer::draw(size_t start, size_t count) {
    glDrawArraysInstancedBaseInstance(GL_TRIANGLE_STRIP, 0, 4, count, start);
}

size_t SpriteBuffer::index() {
    return mIndex;
}

size_t SpriteBuffer::size() {
    return mSize;
}
