#include "MnistDataset.hpp"

#include <scm/scm_filesystem.hpp>
#include <httplib/httplib.h>
#include <fmt/format.h>
#include <zlib.h>

#include "details/Exception.hpp"


namespace {
    std::string bytes_to_str(uint64_t bytes) {
        if (bytes < (1LLU << 10))
            return fmt::format("{}B", bytes);
        else if (bytes < (1LLU << 20))
            return fmt::format("{:.2f}KiB", bytes / 1024.0);
        else if (bytes < (1LLU << 30))
            return fmt::format("{:.2f}MiB", bytes / (1024.0 * 1024.0));
        else
            return fmt::format("{:.2f}GiB", bytes / (1024.0 * 1024.0 * 1024.0));
    }

    bool has_postfix(const std::string& what, const std::string& postfix) {
        return postfix.size() <= what.size() &&
               std::equal(postfix.rbegin(), postfix.rend(), what.rbegin());
    }

    auto create_decompress_callback(const std::string& filename) {
        return [filename](uint64_t length, uint64_t total) {
            fmt::print("\rDecompressing {} \t{} / {} \t{:.2f}%",
                       filename, bytes_to_str(length), bytes_to_str(total), (length * 100.0) / total);
            std::flush(std::cout);

            if (length == total)
                std::cout << std::endl;
        };
    }

    std::string gz_decompress(const std::string& data, const std::function<void(uint64_t, uint64_t)>& progress = {}) {
        auto throw_zlib = [](int error_code, z_streamp to_free = nullptr) {
            const char* msg = "zlib: unknown error";

            switch (error_code) {
                case Z_MEM_ERROR:
                    throw std::bad_alloc();
                case Z_STREAM_ERROR:
                    msg = "zlib: stream error";
                    break;
                case Z_BUF_ERROR:
                    msg = "zlib: buffer error";
                    break;
                case Z_DATA_ERROR:
                    msg = "zlib: data error";
                    break;
                case Z_VERSION_ERROR:
                    msg = "zlib: zlib.h and zlib library versions do not match";
                    break;
                default:
                    break;
            }

            inflateEnd(to_free);
            throw std::runtime_error(msg);
        };

        z_stream stream;

        constexpr const size_t chunk_size = 16384;

        Bytef in [chunk_size];
        Bytef out[chunk_size];
        size_t count = 0;

        std::string res;
        res.reserve(data.capacity());

        stream.zalloc = Z_NULL;
        stream.zfree  = Z_NULL;
        stream.opaque = Z_NULL;
        stream.avail_in = 0;
        stream.next_in = Z_NULL;

        int rc = inflateInit2(&stream, 16 + MAX_WBITS);
        if (rc != Z_OK)
            throw_zlib(rc);

        do {
            stream.avail_in = uInt(data.size() - count >= chunk_size ? chunk_size : data.size() - count);
            memcpy(in, data.data() + count, stream.avail_in);
            count += stream.avail_in;

            if (progress)
                progress(count, data.size());

            if (stream.avail_in == 0)
                break;

            stream.next_in = in;

            do {
                stream.avail_out = chunk_size;
                stream.next_out = out;

                rc = inflate(&stream, Z_NO_FLUSH);

                if (rc != Z_OK && rc != Z_STREAM_END) {
                    if (rc == Z_NEED_DICT)
                        rc = Z_DATA_ERROR;

                    throw_zlib(rc, &stream);
                }

                auto have = chunk_size - stream.avail_out;
                res.append(reinterpret_cast<char*>(out), have);

            }
            while (stream.avail_out == 0);
        }
        while (rc != Z_STREAM_END);

        inflateEnd(&stream);

        return std::move(res);
    }
}

nnw::MnistDataset::MnistDataset(const StringT& data_path, const StringT& labels_path) {
    // Load data
    {
        auto ds = Reader(data_path);

        if (has_postfix(data_path, ".gz")) {
            auto decompressed = gz_decompress(ds.read<StringT>(ds.size()), create_decompress_callback(data_path));
            ds = Reader(decompressed.data(), decompressed.size());
        }

        if (ds.read<uint32_t>() != 0x03080000)
            throw Exception("MnistDataset::MnistDataset(): Wrong magic number!");

        size_t size = byte_swap(ds.read<uint32_t>());
        _data.reserve(size);
        _width  = byte_swap(ds.read<uint32_t>());
        _height = byte_swap(ds.read<uint32_t>());

        for (size_t i = 0; i < size; ++i) {
            auto map = fft::ColorMap8F(_width, _height);

            for (size_t j = 0; j < _height; ++j)
                for (size_t k = 0; k < _width; ++k)
                    map[j][k] = ds.read<uint8_t>() / 255.f;

            _data.push_back(std::move(map));
        }
    }

    // Load labels
    {
        auto ds = Reader(labels_path);

        if (has_postfix(labels_path, ".gz")) {
            auto decompressed = gz_decompress(ds.read<StringT>(ds.size()), create_decompress_callback(data_path));
            ds = Reader(decompressed.data(), decompressed.size());
        }

        if (ds.read<uint32_t>() != 0x01080000)
            throw Exception("MnistDataset::MnistDataset(): Wrong magic number!");

        if (byte_swap(ds.read<uint32_t>()) != _data.size())
            throw Exception("MnistDataset::MnistDataset(): Labels count != images count");

        _labels.reserve(_data.size());

        for (size_t i = 0; i < _data.size(); ++i)
            _labels.push_back(ds.read<uint8_t>());
    }
}

void nnw::MnistDataset::save_tga(const StringT& dir, size_t count) const {
    scm::fs::create_dir(dir);

    for (size_t i = 0; i < count; ++i) {
        StringT name = StringT("digit-") + std::to_string(i) + ".tga";

        auto image = fft::TruevisionImage(fft::TruevisionImage::Type::Monochrome);
        image.from_color_map(_data[i]);
        image.save(dir + name);

        if (i > count)
            break;
    }
    auto sr = Writer();

    sr.write(StringT("labels:\n"));
    for (size_t i = 0; i < count; ++i) {
        sr.write<uint8_t>(_labels[i] + '0');
        sr.write('\n');
    }

    sr.attach_to(dir + "digit-labels.txt");
}

#define TRAIN_IMAGES_NAME "train-images-idx3-ubyte.gz"
#define TRAIN_LABELS_NAME "train-labels-idx1-ubyte.gz"
#define TEST_IMAGES_NAME "t10k-images-idx3-ubyte.gz"
#define TEST_LABELS_NAME "t10k-labels-idx1-ubyte.gz"
#define URL "yann.lecun.com"
#define URL_POSTFIX "/exdb/mnist/"

auto nnw::MnistDataset::remote_load() -> Dataset<MnistDataset> {
    auto files = scm::fs::list_files(scm::fs::current_path());

    auto cached_load = [&files](const char* name) {
        if (std::find(files.begin(), files.end(), name) == files.end()) {
            auto client = httplib::Client(URL);

            std::string compressed_data;
            std::string path = std::string(URL_POSTFIX) + name;

            auto res = client.Get(path.data(),
                [&](const char* data, uint64_t length) {
                    compressed_data.append(data, length);
                    return true;
                },
                [&path](uint64_t length, uint64_t total) {
                    fmt::print("\rLoad " URL "{} \t{} / {} \t{:.2f}%",
                            path, bytes_to_str(length), bytes_to_str(total), (length * 100.0) / total);
                    std::flush(std::cout);

                    if (length == total)
                        std::cout << std::endl;
                    return true;
                }
            );

            if (!res)
                throw std::runtime_error(std::string("Error occurred during loading ") + URL + path);

            Writer(scm::fs::current_path() + name).write(compressed_data);
        }
    };

    cached_load(TRAIN_IMAGES_NAME);
    cached_load(TRAIN_LABELS_NAME);
    cached_load(TEST_IMAGES_NAME);
    cached_load(TEST_LABELS_NAME);

    return {{TRAIN_IMAGES_NAME, TRAIN_LABELS_NAME}, {TEST_IMAGES_NAME, TEST_LABELS_NAME}};
}
