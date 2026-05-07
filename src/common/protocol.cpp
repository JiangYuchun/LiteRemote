#include "literemote/protocol.h"

#include <limits>
#include <stdexcept>

namespace literemote::protocol {

bool IsValid(const PacketHeader& header) noexcept {
    return header.magic == kMagic && header.version == kProtocolVersion &&
           header.payload_size <= (64u * 1024u * 1024u);
}

bool IsValid(const CursorPacketHeader& header) noexcept {
    if (header.format != CursorFormat::Argb32Premultiplied) {
        return false;
    }
    if (header.width == 0 || header.height == 0 || header.pitch == 0) {
        return header.bitmap_size == 0;
    }
    const auto minimum_pitch = header.width * 4u;
    if (header.pitch < minimum_pitch) {
        return false;
    }
    const auto expected_size = static_cast<std::uint64_t>(header.pitch) * header.height;
    return expected_size <= std::numeric_limits<std::uint32_t>::max() &&
           header.bitmap_size == static_cast<std::uint32_t>(expected_size);
}

PacketHeader MakeHeader(Plane plane, std::uint64_t sequence, std::uint64_t timestamp_us,
                        std::uint32_t payload_size) noexcept {
    PacketHeader header{};
    header.plane = plane;
    header.sequence = sequence;
    header.timestamp_us = timestamp_us;
    header.payload_size = payload_size;
    return header;
}

CursorPacket MakeCursorPacket(CursorPacketHeader header, std::span<const std::byte> bitmap) {
    if (bitmap.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("cursor bitmap is too large");
    }

    header.bitmap_size = static_cast<std::uint32_t>(bitmap.size());
    if (!IsValid(header)) {
        throw std::invalid_argument("invalid cursor packet header");
    }

    CursorPacket packet{};
    packet.header = header;
    packet.bitmap.assign(bitmap.begin(), bitmap.end());
    return packet;
}

} // namespace literemote::protocol
