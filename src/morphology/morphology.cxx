#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"

#include "mainhelper1.h"
#include <itksys/SystemTools.hxx>

extern bool Morphology2D(
  const std::string & componentType,
  const unsigned int & Dimension,
  const std::string & inputFileName,
  const std::string & outputFileName,
  const std::string & operation,
  const std::string & type,
  const std::string & boundaryCondition,
  const std::vector<unsigned int> & radius,
  const std::vector<std::string> & bin );
extern bool Morphology3D(
  const std::string & componentType,
  const unsigned int & Dimension,
  const std::string & inputFileName,
  const std::string & outputFileName,
  const std::string & operation,
  const std::string & type,
  const std::string & boundaryCondition,
  const std::vector<unsigned int> & radius,
  const std::vector<std::string> & bin );

//-------------------------------------------------------------------------------------

int main( int argc, char *argv[] )
{
  /** Check arguments for help. */
  if ( argc < 5 )
  {
    PrintHelp();
    return 1;
  }

  /** Create a command line argument parser. */
  itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
  parser->SetCommandLineArguments( argc, argv );

  /** Get arguments. */
  std::string inputFileName = "";
  bool retin = parser->GetCommandLineArgument( "-in", inputFileName );

  std::string operation = "";
  bool retop = parser->GetCommandLineArgument( "-op", operation );
  operation = itksys::SystemTools::UnCapitalizedWords( operation );

  std::string type = "Grayscale";
  bool rettype = parser->GetCommandLineArgument( "-type", type );
  type = itksys::SystemTools::UnCapitalizedWords( type );

  std::string boundaryCondition = "";
  bool retbc = parser->GetCommandLineArgument( "-bc", boundaryCondition );

  std::vector<unsigned int> radius;
  bool retr = parser->GetCommandLineArgument( "-r", radius );

  std::string outputFileName =
    itksys::SystemTools::GetFilenameWithoutLastExtension( inputFileName );
  std::string ext =
    itksys::SystemTools::GetFilenameLastExtension( inputFileName );
  outputFileName += "_" + operation + "_" + type + ext;
  bool retout = parser->GetCommandLineArgument( "-out", outputFileName );

  std::vector<std::string> bin;
  bool retbin = parser->GetCommandLineArgument( "-bin", bin );

  /** Check if the required arguments are given. */
  if ( !retin )
  {
    std::cerr << "ERROR: You should specify \"-in\"." << std::endl;
    return 1;
  }
  if ( !retop )
  {
    std::cerr << "ERROR: You should specify \"-op\"." << std::endl;
    return 1;
  }
  if ( !retr )
  {
    std::cerr << "ERROR: You should specify \"-r\"." << std::endl;
    return 1;
  }

  /** Check for valid input options. */
  if ( operation != "erosion" 
    && operation != "dilation"
    && operation != "opening"
    && operation != "closing" )
  {
    std::cerr << "ERROR: \"-op\" should be one of {erosion, dilation, opening, closing}." << std::endl;
    return 1;
  }
  if ( type != "grayscale" && type != "binary" && type != "parabolic" )
  {
    std::cerr << "ERROR: \"-type\" should be one of {grayscale, binary, parabolic}." << std::endl;
    return 1;
  }
  if ( retbin && bin.size() != 3 )
  {
    std::cerr << "ERROR: \"-bin\" should contain three value: foreground, background, erosion." << std::endl;
    return 1;
  }
  
  /** Determine image properties. */
  std::string componentType = "short";
  std::string pixelType; //we don't use this
  unsigned int Dimension = 3;
  unsigned int numberOfComponents = 1;
  std::vector<unsigned int> imagesize( Dimension, 0 );
  int retgip = GetImageProperties(
    inputFileName,
    pixelType,
    componentType,
    Dimension,
    numberOfComponents,
    imagesize );
  if ( retgip !=0 )
  {
    return 1;
  }
  
  /** Let the user overrule this */
  bool retopct = parser->GetCommandLineArgument( "-opct", componentType );

  if ( numberOfComponents > 1 )
  { 
    std::cerr << "ERROR: The number of components is larger than 1!" << std::endl;
    std::cerr << "Vector images are not supported!" << std::endl;
    return 1;
  }
  
  /** Get rid of the possible "_" in ComponentType. */
  ReplaceUnderscoreWithSpace( componentType );

  /** Check radius. */
  if ( retr )
  {
    if ( radius.size() != Dimension && radius.size() != 1 )
    {
      std::cout << "ERROR: The number of radii should be 1 or Dimension." << std::endl;
      return 1;
    }
  }

  /** Get the radius. */
  std::vector<unsigned int> Radius( Dimension, radius[ 0 ] );
  if ( retr && radius.size() == Dimension )
  {
    for ( unsigned int i = 1; i < Dimension; i++ )
    {
      Radius[ i ] = radius[ i ];
      if ( Radius[ i ] < 1 )
      {
        std::cout << "ERROR: No nonpositive numbers are allowed in radius." << std::endl;
        return 1;
      }
    }
  }
  
  /** Run the program. */
  bool supported = false;
  try
  {
    if ( Dimension == 2 )
    {
      supported = Morphology2D( componentType, Dimension,
        inputFileName, outputFileName, operation, type,
        boundaryCondition, Radius, bin );
    }
    else if ( Dimension == 3 )
    {
      supported = Morphology3D( componentType, Dimension,
        inputFileName, outputFileName, operation, type,
        boundaryCondition, Radius, bin );
    }
  }
  catch( itk::ExceptionObject & e )
  {
    std::cerr << "Caught ITK exception: " << e << std::endl;
    return 1;
  }

  /** Check if this image type was supported. */
  if ( !supported )
  {
    std::cerr << "ERROR: this combination of pixel type and dimension is not supported!" << std::endl;
    std::cerr
      << "pixel (component) type = " << componentType
      << " ; dimension = " << Dimension
      << std::endl;
    return 1;
  }
  
  /** End program. */
  return 0;

} // end main
