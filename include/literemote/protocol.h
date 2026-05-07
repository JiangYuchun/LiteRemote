#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace literemote::protocol {

constexpr std::string_view kProtocolName = "LiteRemote";
constexpr std::uint16_t kProtocolVersion = 1;
constexpr std::uint32_t kMagic = 0x4C52544Du; // 'LRTM'

enum class Plane : std::uint8_t {
    Video = 1,
    Cursor = 2,
    Input = 3,
    Control = 4,
};

enum class CursorFormat : std::uint8_t {
    Argb32Premultiplied = 1,
};

enum class VideoCodec : std::uint8_t {
    RawBgra = 1,
    Mjpeg = 2,
    H264 = 3,
};

struct PacketHeader {
    std::uint32_t magic{kMagic};
    std::uint16_t version{kProtocolVersion};
    Plane plane{Plane::Control};
    std::uint8_t flags{0};
    std::uint64_t sequence{0};
    std::uint64_t timestamp_us{0};
    std::uint32_t payload_size{0};
};

struct CursorPacketHeader {
    std::uint64_t cursor_id{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t pitch{0};
    std::int32_t hotspot_x{0};
    std::int32_t hotspot_y{0};
    std::int32_t pos_x{0};
    std::int32_t pos_y{0};
    std::uint8_t visible{1};
    CursorFormat format{CursorFormat::Argb32Premultiplied};
    std::uint16_t reserved{0};
    std::uint32_t bitmap_size{0};
};

struct CursorPacket {
    CursorPacketHeader header{};
    std::vector<std::byte> bitmap;
};

struct VideoPacketHeader {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t pitch{0};
    VideoCodec codec{VideoCodec::RawBgra};
    std::array<std::uint8_t, 3> reserved{};
    std::uint32_t frame_size{0};
};

[[nodiscard]] bool IsValid(const PacketHeader& header) noexcept;
[[nodiscard]] bool IsValid(const CursorPacketHeader& header) noexcept;
[[nodiscard]] PacketHeader MakeHeader(Plane plane, std::uint64_t sequence, std::uint64_t timestamp_us,
                                      std::uint32_t payload_size) noexcept;
[[nodiscard]] CursorPacket MakeCursorPacket(CursorPacketHeader header, std::span<const std::byte> bitmap);

} // namespace literemote::protocol
