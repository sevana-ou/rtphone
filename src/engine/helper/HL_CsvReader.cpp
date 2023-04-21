#include "HL_CsvReader.h"
#include "HL_String.h"

// --------- CsvFile ----------------
CsvReader::CsvReader(std::istream& stream)
  :mInputStream(stream)
{}

CsvReader::~CsvReader()
{}

std::istream& CsvReader::stream() const
{
  return mInputStream;
}

bool CsvReader::readLine(std::vector<std::string>& cells)
{
  cells.clear();
  std::string line;
  if (!std::getline(mInputStream, line))
    return false;
  strx::trim(line);
  if (line.empty())
      return false;

  strx::split(line, cells, ",;");
  return true;
}
