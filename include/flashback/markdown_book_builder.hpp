#pragma once

#include <flashback/resource_builder.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <regex>

namespace flashback
{

class markdown_book_builder final : public resource_builder
{
public:
    explicit markdown_book_builder(std::filesystem::path const&);
    virtual ~markdown_book_builder();

    virtual void reset() override;
    virtual std::shared_ptr<resource> result() const override;

    ///
    /// \brief Read Content of Title from Buffer
    ///
    /// Resource titles in markdown files (ones that belong to Flashback)
    /// are usually at the first line of the markdown file, right after the
    /// first `#` character.\n
    /// So it should be clear that the first thing being checked in markdown
    /// files are resource titles.
    ///
    /// These titles are expected to be surrounded by square brackets, which
    /// shape a link in markdown syntax.\n
    /// So the only useful part of it is within square brackets.\n
    /// The title then, will be matched through a regular expression pattern.
    ///
    virtual void read_title() override;

    ///
    /// \brief Read Content of Chapters from Buffer
    ///
    /// Chapters are expected to be formed in header size two in markdown syntax.\n
    /// That is, the chapter indicator should be right after `##` characters.
    ///
    /// This indicator triggers that the buffer should be read as if it has
    /// notes for the rest of the file.
    ///
    /// When indicator was read, chapter number should be read and the number of
    /// all chapters should be checked each time, and if all total numbers were
    /// the same, then the total number of chapters are correct.
    ///
    virtual void read_chapters() override;

protected:
    virtual void read_note(std::string const&) override;

private:
    std::shared_ptr<resource> _resource;
    std::ifstream _buffer;
};

} // flashback
