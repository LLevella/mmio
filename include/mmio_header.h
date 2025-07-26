
namespace mmio {
enum class StorageTypes { undefined, dense, sparse };
enum class DataTypes { undefined, real, integer, uinteger, complex, pattern };
enum class MatrixTypes {
  undefined,
  general,
  symmetric,
  hermitian,
  skewSymmetric
};
enum class TriangleTypes { undefined, lower, upper, both, diagonal };

class Header {
  static const std::string MMstart;
  static const std::string dataDeffinition;
  static const std::map<std::string, StorageTypes> storageTypes;
  static const std::map<std::string, DataTypes> dataTypes;
  static const std::map<std::string, MatrixTypes> matrixTypes;
  StorageTypes stype;
  DataTypes dtype;
  MatrixTypes mtype;

 public:
  Header()
      : stype(StorageTypes::undefined),
        dtype(DataTypes::undefined),
        mtype(MatrixTypes::undefined){};
};

const std::string Header::MMstart = "%%matrixmarket";
const std::string Header::dataDeffinition = "matrix";
const std::map<std::string, StorageTypes> Header::storageTypes = {
    {"array", StorageTypes::dense}, {"coordinate", StorageTypes::sparse}};
const std::map<std::string, DataTypes> Header::dataTypes = {
    {"complex", DataTypes::complex},
    {"real", DataTypes::real},
    {"integer", DataTypes::integer},
    {"unsigned-integer", DataTypes::uinteger},
    {"pattern", DataTypes::pattern}};
const std::map<std::string, MatrixTypes> Header::matrixTypes = {
    {"general", MatrixTypes::general},
    {"symmetric", MatrixTypes::symmetric},
    {"hermitian", MatrixTypes::hermitian},
    {"skew-symmetric", MatrixTypes::skewSymmetric}};
}  // namespace mmio