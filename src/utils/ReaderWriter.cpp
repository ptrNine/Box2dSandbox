#include "ReaderWriter.hpp"

Reader::Reader(Reader&& r) noexcept: _buf(std::move(r._buf)), _is(nullptr) {
    if (_buf.index() == 0)
        _is.rdbuf(&std::get<std::filebuf>(_buf));
    else
        _is.rdbuf(&std::get<std::stringbuf>(_buf));
}

Reader& Reader::operator=(Reader&& r) noexcept{
    _buf = std::move(r._buf);

    if (_buf.index() == 0)
        _is.rdbuf(&std::get<std::filebuf>(_buf));
    else
        _is.rdbuf(&std::get<std::stringbuf>(_buf));

    return *this;
}

Reader::Reader(const void* data, size_t size) :
        _buf(std::stringbuf(std::string(reinterpret_cast<const char*>(data), size), std::ios::in | std::ios::binary)),
        _is(nullptr)
{
    _is.rdbuf(&std::get<std::stringbuf>(_buf));
}

Reader::Reader(const std::string& path) : _buf(std::filebuf()), _is(nullptr) {
    auto& fb = std::get<std::filebuf>(_buf);
    fb.open(path, std::ios::in | std::ios::binary);

    if (!fb.is_open())
        throw std::runtime_error("Can't open file '" + path + "'");

    _is.rdbuf(&fb);
}

Reader& Reader::operator >> (Writer& writer) {
    auto saved = _is.tellg();

    if (_is.eof()) {
        _is.clear();
        _is.seekg(0, std::ios::beg);
    }

    char chunk[CHUNK_SIZE];

    while (!_is.eof()) {
        read(chunk, CHUNK_SIZE);
        writer.write(chunk, gcount());
    }

    _is.clear();
    _is.seekg(saved);

    return *this;
}

Reader& Reader::operator >> (Writer&& writer) {
    auto w = std::move(writer);
    return *this >> w;
}

Writer::Writer(Writer&& w) noexcept: _buf(std::move(w._buf)), _os(nullptr) {
    if (_buf.index() == 0)
        _os.rdbuf(&std::get<std::filebuf>(_buf));
    else
        _os.rdbuf(&std::get<std::stringbuf>(_buf));
}

Writer& Writer::operator=(Writer&& w) noexcept {
    _buf = std::move(w._buf);

    if (is_file_streamer())
        _os.rdbuf(&std::get<std::filebuf>(_buf));
    else
        _os.rdbuf(&std::get<std::stringbuf>(_buf));

    return *this;
}

Writer::Writer() :
        _buf(std::stringbuf(std::string(), _open_flags)),
        _os(nullptr)
{
    _os.rdbuf(&std::get<std::stringbuf>(_buf));
}

Writer::Writer(const std::string& path) : _buf(std::filebuf()), _os(nullptr) {
    auto& fb = std::get<std::filebuf>(_buf);
    fb.open(path, _open_flags);

    if (!fb.is_open())
        throw std::runtime_error("Can't create file '" + path + "'");

    _os.rdbuf(&fb);
}

void Writer::attach_to(const std::string& path) {
    _os.flush();

    auto create_fbuf = [this](const std::string& path) {
        _buf = std::filebuf();
        auto& fb = std::get<std::filebuf>(_buf);
        fb.open(path, _open_flags);

        if (!fb.is_open())
            throw std::runtime_error("Can't create file '" + path + "'");
        _os.rdbuf(&fb);
        _os.clear();
        _os.seekp(0, std::ios::beg);
    };

    if (is_buffer_streamer()) {
        auto strbuf = std::move(std::get<std::stringbuf>(_buf));

        create_fbuf(path);

        auto str = strbuf.str();
        write(str.data(), str.size());
    }
    else {
        auto filebuf = std::move(std::get<std::filebuf>(_buf));
        filebuf.pubseekpos(0, _open_flags);

        create_fbuf(path);

        auto is = std::istream(&filebuf);
        char chunk[CHUNK_SIZE];
        do {
            is.read(chunk, CHUNK_SIZE);
            write(chunk, static_cast<size_t>(is.gcount()));
        } while (!is.eof());
    }
}

void Writer::detach() {
    _os.flush();

    auto create_sbuf = [this]() {
        _buf = std::stringbuf();
        auto& sb = std::get<std::stringbuf>(_buf);

        _os.rdbuf(&sb);
        _os.clear();
        _os.seekp(0, std::ios::beg);
    };

    if (is_file_streamer()) {
        auto filebuf = std::move(std::get<std::filebuf>(_buf));
        filebuf.pubseekpos(0, _open_flags);

        create_sbuf();

        auto is = std::istream(&filebuf);
        char chunk[CHUNK_SIZE];
        do {
            is.read(chunk, CHUNK_SIZE);
            write(chunk, static_cast<size_t>(is.gcount()));
        } while (!is.eof());
    }
}