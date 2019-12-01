#pragma once

#include "details/Types.hpp"
#include "../utils/TruevisionImage.hpp"

namespace nnw {
    using StringT = std::string;
    using FloatT  = float;

    class MnistDataset {
    public:
        MnistDataset(const StringT& data_path, const StringT& labels_path);

        auto data() const -> const scl::Vector<fft::ColorMap8F>& {
            return _data;
        }

        auto labels() const -> const scl::Vector<uint8_t>& {
            return _labels;
        }

        size_t count() {
            return _data.size();
        }

        void save_tga(const StringT& dir, size_t count = 100) const;

    public:
        template <typename T>
        struct Dataset { T trainset, testset; };

        static auto remote_load() -> Dataset<MnistDataset>;

    private:
        size_t _width  = 0;
        size_t _height = 0;
        scl::Vector<fft::ColorMap8F> _data;
        scl::Vector<uint8_t>         _labels;
    };
}