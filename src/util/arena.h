#include <algorithm>
#include <array>
#include <cstddef>

#ifdef GAME_BASE_SHARED
#include "../../../src/util/zpp_bits.h"
#endif

template <size_t CAP, typename T>
class Arena 
{
#ifdef GAME_BASE_SHARED
    friend zpp::bits::access;
	using serialize = zpp::bits::members<7>;
#endif

    std::array<T, CAP> _data;
    size_t _count;
    size_t _nextId;
    std::array<size_t, CAP> _slotOfId;
    std::array<size_t, CAP> _idOfSlot;
    std::array<size_t, CAP> _freeIds;
    size_t _freeCount;

public:
    
    Arena() :
        _count(0),
        _nextId(0),
        _freeCount(0)
    {
        _slotOfId.fill(invalid_index());
        _idOfSlot.fill(invalid_index());
    }

    T* data() {
        return _data.data();
    }

    size_t size() {
        return CAP * sizeof(T);
    }

    size_t acquire(const T& obj, size_t count = 1) {
        (void)count;
        if (_count >= CAP) {
            return invalid_index();
        }
        size_t id = invalid_index();
        if (_freeCount > 0) {
            id = _freeIds[--_freeCount];
        } else {
            id = _nextId++;
            if (id >= CAP) {
                return invalid_index();
            }
        }
        const size_t slot = _count++;
        _data[slot] = obj;
        _idOfSlot[slot] = id;
        _slotOfId[id] = slot;
        return id;
    }

    T& at_raw(size_t idx) {
        return _data[idx];
    } 

    T& at(size_t idx) {
        return _data[_slotOfId[idx]];
    }    

    const T& get(size_t idx) const {
        return _data[_slotOfId[idx]];
    }

    size_t count() const {return _count;}
    size_t capacity() {return CAP;}

    void clear() {
        _count = 0;
        _nextId = 0;
        _freeCount = 0;
        _slotOfId.fill(invalid_index());
        _idOfSlot.fill(invalid_index());
    }

    bool has(const T& obj) {
        bool found = false;
        for (size_t i = 0; i < count(); ++i) {
            if (_data[i].row == obj.row && _data[i].col == obj.col) {
                found = true;
                break;
            }
        }
        return found;
    }

    bool remove(size_t idx) {
        if (idx >= CAP) {
            return false;
        }
        const size_t slot = _slotOfId[idx];
        if (slot == invalid_index() || slot >= _count) {
            return false;
        }
        const size_t last_slot = _count - 1;
        if (slot != last_slot) {
            _data[slot] = _data[last_slot];
            const size_t moved_id = _idOfSlot[last_slot];
            _idOfSlot[slot] = moved_id;
            _slotOfId[moved_id] = slot;
        }
        _idOfSlot[last_slot] = invalid_index();
        _slotOfId[idx] = invalid_index();
        _count--;
        _freeIds[_freeCount++] = idx;
        return true;
    }

private:
    static constexpr size_t invalid_index() {
        return static_cast<size_t>(-1);
    }

};
