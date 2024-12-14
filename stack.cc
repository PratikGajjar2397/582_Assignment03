#include <vector>
#include <string>
#include <cassert>
#include <cstring>
#include <memory>

class Stack {
private:
    struct Chunk {
        std::unique_ptr<char[]> data;
        Chunk* next;

        Chunk(size_t size) : data(std::make_unique<char[]>(size)), next(nullptr) {}
    };

    Chunk* currentChunk;
    Chunk* freeChunks;
    size_t currentIndex;
    size_t chunkElemMax;
    size_t elemSize;
    size_t totalElements;

    void* getLastElement() {
        return currentChunk->data.get() + (elemSize * currentIndex);
    }

    size_t calculateChunkElemMax(size_t elemSize, size_t chunkSize) {
        const size_t elemSizeMin = elemSize * 32; // CHUNK_ELEM_MIN
        while (chunkSize <= elemSizeMin) {
            chunkSize <<= 1;
        }
        chunkSize -= sizeof(Chunk);
        return chunkSize / elemSize;
    }

public:
    Stack(size_t elemSize, size_t chunkSize = (1 << 16)) // Default chunk size: 64KB
        : currentChunk(nullptr), freeChunks(nullptr), currentIndex(0), elemSize(elemSize), totalElements(0) {
        chunkElemMax = calculateChunkElemMax(elemSize, chunkSize);
        currentIndex = chunkElemMax - 1;
    }

    ~Stack() {
        clearChunks(currentChunk);
        clearChunks(freeChunks);
    }

    void clearChunks(Chunk* chunk) {
        while (chunk) {
            Chunk* next = chunk->next;
            delete chunk;
            chunk = next;
        }
    }

    void push(const void* src) {
        currentIndex++;
        if (currentIndex == chunkElemMax) {
            Chunk* newChunk = freeChunks ? freeChunks : new Chunk(elemSize * chunkElemMax);
            if (freeChunks) freeChunks = freeChunks->next;
            newChunk->next = currentChunk;
            currentChunk = newChunk;
            currentIndex = 0;
        }
        std::memcpy(getLastElement(), src, elemSize);
        totalElements++;
    }

    void pop(void* dst) {
        assert(totalElements > 0);
        std::memcpy(dst, getLastElement(), elemSize);
        discard();
    }

    void discard() {
        assert(totalElements > 0);
        totalElements--;
        if (--currentIndex == static_cast<size_t>(-1)) {
            Chunk* oldChunk = currentChunk;
            currentChunk = currentChunk->next;
            oldChunk->next = freeChunks;
            freeChunks = oldChunk;
            currentIndex = chunkElemMax - 1;
        }
    }

    bool isEmpty() const {
        return totalElements == 0;
    }

    size_t count() const {
        return totalElements;
    }

    void clear() {
        if (freeChunks) {
            Chunk* lastFreeChunk = freeChunks;
            while (lastFreeChunk->next) {
                lastFreeChunk = lastFreeChunk->next;
            }
            lastFreeChunk->next = currentChunk;
        } else {
            freeChunks = currentChunk;
        }
        currentChunk = nullptr;
        totalElements = 0;
        currentIndex = chunkElemMax - 1;
    }
};
