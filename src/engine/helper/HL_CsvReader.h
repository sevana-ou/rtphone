#ifndef __HL_CSVREADER_H
#define __HL_CSVREADER_H

#include <string>
#include <istream>
#include <vector>

class CsvReader
{
public:
  CsvReader(std::istream& stream);
  ~CsvReader();

  void setStream(std::istream& input);
  std::istream& stream() const;

  bool readLine(std::vector<std::string>& cells);

protected:
  std::istream& mInputStream;
};

#endif
