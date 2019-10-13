#pragma once

#include <string>
#include <cstring>
#include <vector>

#include "SimpleSerializer.hpp"

namespace nnw {
    using FloatT  = float;
    using StringT = std::string;

    struct Color24 {
        Color24() = default;
        Color24(uint8_t ir, uint8_t ig, uint8_t ib): r(ir), g(ig), b(ib) {}
        Color24(const struct ColorFloat24& c);

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
    };

    struct ColorFloat24 {
        static FloatT clamp(FloatT in) {
            return in > FloatT(1.0) ? FloatT(1.0) : (in < FloatT(0.0) ? FloatT(0.0) : in);
        }
        ColorFloat24() = default;
        ColorFloat24(const Color24& c): r(c.r / 255.f), g(c.g / 255.f), b(c.b / 255.f) {}
        ColorFloat24(FloatT ir, FloatT ig, FloatT ib): r(clamp(ir)), g(clamp(ig)), b(clamp(ib)) {}

        FloatT r = 0;
        FloatT g = 0;
        FloatT b = 0;
    };

    inline Color24::Color24(const ColorFloat24 &c): r(c.clamp(c.r) * 255), g(c.clamp(c.g) * 255), b(c.clamp(c.b) * 255) {}


    template <typename T = uint8_t>
    class ColorMap {
    public:
        ColorMap() = default;

        ColorMap(size_t width, size_t height): _width(width), _height(height) {
            _data.resize(_width * _height);
        }

        ColorMap(ColorMap&& map): _width(map._width), _height(map._height), _data(std::move(map._data)) {}

        T* operator[] (size_t i) const {
            return const_cast<T*>(_data.data()) + (i * _width);
        }

        operator bool() const {
            return _data;
        }

        size_t width() const {
            return _width;
        }

        size_t height() const {
            return _height;
        }

        auto& data() const {
            return _data;
        }

        auto& data() {
            return _data;
        }

    private:
        std::vector<T> _data;
        size_t   _width  = 0;
        size_t   _height = 0;
    };

    using ColorMap8   = ColorMap<uint8_t>;
    using ColorMap24  = ColorMap<Color24>;
    using ColorMap8F  = ColorMap<FloatT>;
    using ColorMap24F = ColorMap<ColorFloat24>;


    class TtfException : public std::exception {
    public:
        explicit TtfException(const StringT& error) : _exc(error.data()) {}
        const char* what() const noexcept override { return _exc.data(); }
    private:
        StringT _exc;
    };


    class TruevisionImage {
    public:
        enum class Type : uint8_t {
            TrueColor = 2, Monochrome = 3
        };

        static bool check_type(Type type) {
            return type == Type::TrueColor || type == Type::Monochrome;
        }

        static size_t bytes_per_pixel(Type type) {
            switch (type) {
                case Type::Monochrome: return 1;
                case Type::TrueColor:  return 3;
                default: return 0;
            }
        }

        TruevisionImage() = default;

        TruevisionImage(Type type): _type(type) {}

        TruevisionImage(Type type, size_t width, size_t height) : _type(type), _width(width), _height(height) {
            _data = new uint8_t[_width * _height * bytes_per_pixel(_type)]();
        }

        TruevisionImage(const StringT& path) {
            load(path);
        }

        TruevisionImage(const TruevisionImage& image)
                : _type(image._type), _width(image._width), _height(image._height)
        {
            delete [] _data;
            _data = new uint8_t[_width * _height * bytes_per_pixel(_type)];
            std::memcpy(_data, image._data, _width * _height * bytes_per_pixel(_type));
        }

        TruevisionImage& operator=(const TruevisionImage& image) {
            _type   = image._type;
            _width  = image._width;
            _height = image._height;

            delete [] _data;

            _data = new uint8_t[_width * _height * bytes_per_pixel(_type)];
            std::memcpy(_data, image._data, _width * _height * bytes_per_pixel(_type));

            return *this;
        }

        void load(const StringT& path) {
            delete [] _data;

            auto ds = FileDeserializer(path);

            ds.skip(2);
            _type = ds.pop<Type>();

            if (!check_type(_type))
                throw TtfException("Unsupported ttf image type");

            ds.skip(9);
            _width  = ds.pop<uint16_t>();
            _height = ds.pop<uint16_t>();

            auto bpp = ds.pop<uint8_t>();

            if (_type == Type::Monochrome && bpp != 8)
                throw TtfException("Unknown bit per pixel in monochrome image: " + std::to_string(bpp));

            if (_type == Type::TrueColor && bpp != 24)
                throw TtfException("Unsupported bit per pixel in TrueColor image: " + std::to_string(bpp));

            ds.skip(1);

            _data = new uint8_t[_width * _height * bytes_per_pixel(_type)]();
            ds.pop(_data, _width * _height * bytes_per_pixel(_type));
        }

        void save(const StringT& path) const {
            auto serializer = nnw::Serializer();
            // Header
            serializer.push<uint8_t>(0); // Identifier (no ID)
            serializer.push<uint8_t>(0); // Color map type (no map)
            serializer.push<uint8_t>(static_cast<uint8_t>(_type)); // TrueColor type
            serializer.zero_fill(5); // color map (empty)

            serializer.push<uint16_t>(0); // pos X
            serializer.push<uint16_t>(0); // pos Y
            serializer.push<uint16_t>(static_cast<uint16_t>(_width)); // size X
            serializer.push<uint16_t>(static_cast<uint16_t>(_height)); // size Y
            serializer.push<uint8_t>(static_cast<uint8_t>(bytes_per_pixel(_type) * 8)); // 24 bits per pixel
            serializer.zero_fill(1);      // image descriptor

            serializer.push(_data, _width * _height * bytes_per_pixel(_type));

            // Footer
            serializer.zero_fill(8);
            serializer.push("TRUEVISION-XFILE");
            serializer.push('.');
            serializer.push<uint8_t>(0);

            serializer.save_to(path);
        }

        operator bool() const {
            return _data;
        }

        void init(Type type, size_t width, size_t height) {
            delete [] _data;
            _type   = type;
            _width  = width;
            _height = height;
            _data = new uint8_t[_width * _height * bytes_per_pixel(_type)];
        }

        void from_color_map(const ColorMap8& map) {
            init(Type::Monochrome, map.width(), map.height());

            for (size_t i = 0; i < _height; ++i)
                for (size_t j = 0; j < _width; ++j)
                   (*this)[_height - i - 1][j] = map[i][j];
        }

        void from_color_map(const ColorMap24& map) {
            init(Type::TrueColor, map.width(), map.height());

            if (_type == Type::TrueColor) {
                for (size_t i = 0; i < _height; ++i) {
                    for (size_t j = 0; j < _width; ++j) {
                        (*this)[_height - i - 1][j*3 + 0] = map[i][j].b;
                        (*this)[_height - i - 1][j*3 + 1] = map[i][j].g;
                        (*this)[_height - i - 1][j*3 + 2] = map[i][j].r;
                    }
                }
            }
        }

        void from_color_map(const ColorMap8F& map) {
            init(Type::Monochrome, map.width(), map.height());

            for (size_t i = 0; i < _height; ++i)
                for (size_t j = 0; j < _width; ++j)
                    (*this)[_height - i - 1][j] = uint8_t(ColorFloat24::clamp(map[i][j]) * 255);
        }

        void from_color_map(const ColorMap24F& map) {
            init(Type::TrueColor, map.width(), map.height());

            if (_type == Type::TrueColor) {
                for (size_t i = 0; i < _height; ++i) {
                    for (size_t j = 0; j < _width; ++j) {
                        auto color = Color24(map[i][j]);
                        (*this)[_height - i - 1][j*3 + 0] = color.b;
                        (*this)[_height - i - 1][j*3 + 1] = color.g;
                        (*this)[_height - i - 1][j*3 + 2] = color.r;
                    }
                }
            }
        }

        auto to_color_map8() const {
            auto map = ColorMap8(_width, _height);

            if (_type == Type::Monochrome) {
                for (size_t i = 0; i < _height; ++i)
                    for (size_t j = 0; j < _width; ++j)
                        map[i][j] = (*this)[_height - i - 1][j];
            }
            else if (_type == Type::TrueColor) {
                for (size_t i = 0; i < _height; ++i)
                    for (size_t j = 0; j < _width; ++j)
                        map[i][j] = static_cast<uint8_t>(
                                (size_t((*this)[_height - i - 1][j*3 + 0]) +
                                        (*this)[_height - i - 1][j*3 + 1]  +
                                        (*this)[_height - i - 1][j*3 + 2])
                                        / 3);
            }
            else
                throw TtfException("TruevisionImage::to_color_map8(): Unsupported type");

            return std::move(map);
        }

        auto to_color_map8f() const {
            auto map = ColorMap8F(_width, _height);

            if (_type == Type::Monochrome) {
                for (size_t i = 0; i < _height; ++i)
                    for (size_t j = 0; j < _width; ++j)
                        map[i][j] = (*this)[_height - i - 1][j] / 255.f;
            }
            else if (_type == Type::TrueColor) {
                for (size_t i = 0; i < _height; ++i)
                    for (size_t j = 0; j < _width; ++j)
                        map[i][j] = ((size_t((*this)[_height - i - 1][j*3 + 0]) +
                                     (*this)[_height - i - 1][j*3 + 1]  +
                                     (*this)[_height - i - 1][j*3 + 2]) / 3.f) / 255.f;
            }
            else
                throw TtfException("TruevisionImage::to_color_map8f(): Unsupported type");

            return std::move(map);
        }

        auto to_color_map24() const {
            auto map = ColorMap24(_width, _height);

            if (_type == Type::Monochrome) {
                for (size_t i = 0; i < _height; ++i) {
                    for (size_t j = 0; j < _width; ++j) {
                        map[i][j].r = (*this)[_height - i - 1][j*3];
                        map[i][j].g = (*this)[_height - i - 1][j*3];
                        map[i][j].b = (*this)[_height - i - 1][j*3];
                    }
                }
            }
            else if (_type == Type::TrueColor) {
                for (size_t i = 0; i < _height; ++i) {
                    for (size_t j = 0; j < _width; ++j) {
                        map[i][j].r = (*this)[_height - i - 1][j*3 + 2];
                        map[i][j].g = (*this)[_height - i - 1][j*3 + 1];
                        map[i][j].b = (*this)[_height - i - 1][j*3 + 0];
                    }
                }
            }
            else
                throw TtfException("TruevisionImage::to_color_map24(): Unsupported type");

            return std::move(map);
        }

        auto to_color_map24f() const {
            auto map = ColorMap24F(_width, _height);

            if (_type == Type::Monochrome) {
                for (size_t i = 0; i < _height; ++i)
                    for (size_t j = 0; j < _width; ++j)
                        map[i][j] = ColorFloat24(
                                Color24((*this)[_height - i - 1][j*3],
                                        (*this)[_height - i - 1][j*3],
                                        (*this)[_height - i - 1][j*3]));
            }
            else if (_type == Type::TrueColor) {
                for (size_t i = 0; i < _height; ++i)
                    for (size_t j = 0; j < _width; ++j)
                        map[i][j] = ColorFloat24(
                                Color24((*this)[_height - i - 1][j*3 + 2],
                                        (*this)[_height - i - 1][j*3 + 1],
                                        (*this)[_height - i - 1][j*3 + 0]));
            }
            else
                throw TtfException("TruevisionImage::to_color_map24(): Unsupported type");

            return std::move(map);
        }

        size_t width () const { return _width; }
        size_t height() const { return _height; }
        Type   type  () const { return _type; }

        uint8_t* operator[] (size_t i) const {
            return _data + (i * _width * bytes_per_pixel(_type));
        }

    private:
        uint8_t* _data = nullptr;
        size_t _width  = 0;
        size_t _height = 0;
        Type   _type   = Type::TrueColor;
    };
}