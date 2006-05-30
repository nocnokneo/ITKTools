#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"

#include "itkImage.h"
#include "itkResampleImageFilter.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

//-------------------------------------------------------------------------------------

/** run: A macro to call a function. */
#define run(function,type,dim) \
if ( PixelType == #type && Dimension == dim ) \
{ \
    typedef itk::Image< type, dim > InputImageType; \
    function< InputImageType >( inputFileName, outputFileName, factorOrSpacing, isFactor ); \
}

//-------------------------------------------------------------------------------------

/** Declare ResizeImage. */
template< class InputImageType >
void ResizeImage( std::string inputFileName, std::string outputFileName,
	std::vector<double> factorOrSpacing, bool isFactor );

/** Declare PrintHelp. */
void PrintHelp(void);

//-------------------------------------------------------------------------------------

int main( int argc, char **argv )
{
	/** Check arguments for help. */
	if ( argc < 5 || argc > 13 )
	{
		PrintHelp();
		return 1;
	}

	/** Create a command line argument parser. */
	itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
	parser->SetCommandLineArguments( argc, argv );

	/** Get arguments. */
	std::string	inputFileName = "";
	bool retin = parser->GetCommandLineArgument( "-in", inputFileName );

	std::string	outputFileName = inputFileName.substr( 0, inputFileName.rfind( "." ) );
	outputFileName += "RESIZED.mhd";
	bool retout = parser->GetCommandLineArgument( "-out", outputFileName );

	std::vector<double> factor;
	bool retf = parser->GetCommandLineArgument( "-f", factor );
	bool isFactor = retf;

	std::vector<double> spacing;
	bool retsp = parser->GetCommandLineArgument( "-sp", spacing );

	unsigned int Dimension = 3;
	bool retdim = parser->GetCommandLineArgument( "-dim", Dimension );

	std::string	PixelType = "short";
	bool retpt = parser->GetCommandLineArgument( "-pt", PixelType );

	/** Check if the required arguments are given. */
	if ( !retin )
	{
		std::cerr << "ERROR: You should specify \"-in\"." << std::endl;
		return 1;
	}
	if ( !( retf ^ retsp ) )
	{
		std::cerr << "ERROR: You should specify either \"-f\" or \"-sp\"." << std::endl;
		return 1;
	}

	/** Get rid of the possible "_" in PixelType. */
	ReplaceUnderscoreWithSpace( PixelType );

	/** Check factor and spacing. */
	if ( retf )
	{
		if( factor.size() != Dimension && factor.size() != 1 )
		{
			std::cout << "ERROR: The number of factors should be 1 or Dimension." << std::endl;
			return 1;
		}
	}
	if ( retsp )
	{
		if( spacing.size() != Dimension && spacing.size() != 1 )
		{
			std::cout << "ERROR: The number of spacings should be 1 or Dimension." << std::endl;
			return 1;
		}
	}

	/** Get the factor or spacing. */
	double vector0 = ( retf ? factor[ 0 ] : spacing[ 0 ] );
	std::vector<double> factorOrSpacing( Dimension, vector0 );
	if ( retf && factor.size() == Dimension )
	{
		for ( unsigned int i = 1; i < Dimension; i++ )
		{
			factorOrSpacing[ i ] = factor[ i ];
		}
	}
	if ( retsp && spacing.size() == Dimension )
	{
		for ( unsigned int i = 1; i < Dimension; i++ )
		{
			factorOrSpacing[ i ] = spacing[ i ];
		}
	}

	/** Check factorOrSpacing for negative numbers. */
	for ( unsigned int i = 1; i < Dimension; i++ )
	{
		if ( factorOrSpacing[ i ] < 0.00001 )
		{
			std::cout << "ERROR: No negative numbers are allowed in factor or spacing." << std::endl;
			return 1;
		}
	}

	/** Run the program. */
	try
	{
		run(ResizeImage,unsigned char,2);
		run(ResizeImage,unsigned char,3);
		run(ResizeImage,char,2);
		run(ResizeImage,char,3);
		run(ResizeImage,unsigned short,2);
		run(ResizeImage,unsigned short,3);
		run(ResizeImage,short,2);
		run(ResizeImage,short,3);
	}
	catch( itk::ExceptionObject &e )
	{
		std::cerr << "Caught ITK exception: " << e << std::endl;
		return 1;
	}
	
	/** End program. */
	return 0;

} // end main


	/**
	 * ******************* ResizeImage *******************
	 *
	 * The resize function templated over the input pixel type.
	 */

template< class InputImageType >
void ResizeImage( std::string inputFileName, std::string outputFileName,
	std::vector<double> factorOrSpacing, bool isFactor )
{
	/** Typedefs. */
	typedef itk::ResampleImageFilter< InputImageType, InputImageType >	ResamplerType;
	typedef itk::ImageFileReader< InputImageType >			ReaderType;
	typedef itk::ImageFileWriter< InputImageType >			WriterType;

	typedef typename InputImageType::SizeType					SizeType;
	typedef typename InputImageType::SpacingType			SpacingType;

	const unsigned int Dimension = InputImageType::ImageDimension;

	/** Declarations. */
	typename InputImageType::Pointer inputImage;
	typename ResamplerType::Pointer resampler = ResamplerType::New();
	typename ReaderType::Pointer reader = ReaderType::New();
	typename WriterType::Pointer writer = WriterType::New();

	/** Read in the inputImage. */
	reader->SetFileName( inputFileName.c_str() );
	inputImage = reader->GetOutput();
	inputImage->Update();

	/** Prepare stuff. */
	SpacingType	inputSpacing	= inputImage->GetSpacing();
	SizeType		inputSize			= inputImage->GetLargestPossibleRegion().GetSize();
	SpacingType outputSpacing	= inputSpacing;
	SizeType		outputSize		= inputSize;	
	if ( isFactor )
	{
		for ( unsigned int i = 0; i < Dimension; i++ )
		{
			outputSpacing[ i ] /= factorOrSpacing[ i ];
			outputSize[ i ] = static_cast<unsigned int>( outputSize[ i ] * factorOrSpacing[ i ] );
		}
	}
	else
	{
		for ( unsigned int i = 0; i < Dimension; i++ )
		{
			outputSpacing[ i ] = factorOrSpacing[ i ];
			outputSize[ i ] = static_cast<unsigned int>( inputSpacing[ i ] * inputSize[ i ] / factorOrSpacing[ i ] );
		}
	}

	/** Setup the pipeline.
	 * The resampler has by default an IdentityTransform as transform, and
	 * a LinearInterpolateImageFunction as interpolator.
	 */
	resampler->SetInput( inputImage );
	resampler->SetSize( outputSize );
	resampler->SetDefaultPixelValue( 0 );
	resampler->SetOutputStartIndex( inputImage->GetLargestPossibleRegion().GetIndex() );
	resampler->SetOutputSpacing( outputSpacing );
	resampler->SetOutputOrigin( inputImage->GetOrigin() );

	/** Write the output image. */
	writer->SetFileName( outputFileName.c_str() );
	writer->SetInput( resampler->GetOutput() );
	writer->Update();

} // end ResizeImage


	/**
	 * ******************* PrintHelp *******************
	 */
void PrintHelp()
{
	std::cout << "Usage:" << std::endl << "pxresizeimage" << std::endl;
	std::cout << "\t-in\tinputFilename" << std::endl;
	std::cout << "\t[-out]\toutputFilename, default in + RESIZED.mhd" << std::endl;
	std::cout << "\t[-f]\tfactor" << std::endl;
	std::cout << "\t[-sp]\tspacing" << std::endl;
	std::cout << "\t[-dim]\tdimension, default 3" << std::endl;
	std::cout << "\t[-pt]\tpixelType, default short" << std::endl;
	std::cout << "One of -f and -sp should be given." << std::endl;
	std::cout << "Supported: 2D, 3D, (unsigned) short, (unsigned) char." << std::endl;
} // end PrintHelp
