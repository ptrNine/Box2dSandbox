#pragma once

#include "TruevisionImage.hpp"

namespace nnw {
    using StringT = std::string;
    using FloatT  = float;

    class MnistDataset {
    public:
        MnistDataset(const StringT& data_path, const StringT& labels_path) {
            // Load data
            {
                auto ds = FileDeserializer(data_path);

                if (ds.pop<uint32_t>() != 0x03080000)
                    throw Exception("MnistDataset::MnistDataset(): Wrong magic number!");

                size_t size = swap_endian(ds.pop<uint32_t>());
                _data.reserve(size);
                _width = swap_endian(ds.pop<uint32_t>());
                _height = swap_endian(ds.pop<uint32_t>());

                for (size_t i = 0; i < size; ++i) {
                    auto map = ColorMap8F(_width, _height);

                    for (size_t j = 0; j < _height; ++j)
                        for (size_t k = 0; k < _width; ++k)
                            map[j][k] = ds.pop<uint8_t>() / 255.f;

                    _data.emplace_back(std::move(map));
                }
            }

            // Load labels
            {
                auto ds = FileDeserializer(labels_path);

                if (ds.pop<uint32_t>() != 0x01080000)
                    throw Exception("MnistDataset::MnistDataset(): Wrong magic number!");

                if (swap_endian(ds.pop<uint32_t>()) != _data.size())
                    throw Exception("MnistDataset::MnistDataset(): Labels count != images count");

                _labels.reserve(_data.size());

                for (size_t i = 0; i < _data.size(); ++i)
                    _labels.push_back(ds.pop<uint8_t>());
            }
        }

        auto data() const -> const std::vector<ColorMap8F>& {
            return _data;
        }

        auto labels() const -> const std::vector<uint8_t>& {
            return _labels;
        }

        size_t count() {
            return _data.size();
        }

        void save_tga(const StringT& dir, size_t count = 100) const {
            for (size_t i = 0; i < count; ++i) {
                StringT name = StringT("digit-") + std::to_string(i) + ".tga";

                auto image = TruevisionImage(TruevisionImage::Type::Monochrome);
                image.from_color_map(_data[i]);
                image.save(dir + name);

                if (i > count)
                    break;
            }
            auto sr = Serializer();

            sr.push("labels:\n");
            for (size_t i = 0; i < count; ++i) {
                sr.push<uint8_t>(_labels[i] + '0');
                sr.push('\n');
            }

            sr.write(dir + "digit-labels.txt");
        }

    private:
        size_t _width  = 0;
        size_t _height = 0;
        std::vector<ColorMap8F> _data;
        std::vector<uint8_t>    _labels;
    };
}