#include "recognition_helpers.h"
#include "output.h"
#include "chemical_structure_recognizer.h"
#include "molecule.h"
#include "prefilter_entry.h"
#include "superatom_expansion.h"
#include "log_ext.h"
#include "image_utils.h"

namespace recognition_helpers
{
	void dumpVFS(imago::VirtualFS& vfs, const std::string& filename)
	{
		// store all the vfs contents in one single file (including html, images, etc)
		if (!vfs.empty())
		{
			imago::FileOutput filedump(filename.c_str());
			std::vector<char> data;		
			vfs.getData(data); 
			filedump.write(&data.at(0), data.size());
		}
	}

	void applyConfig(bool verbose, imago::Settings& vars, const std::string& config)
	{
		if (!config.empty())
		{
			if (verbose)
				printf("Loading configuration cluster [%s]... ", config.c_str());

			bool result = vars.forceSelectCluster(config);

			if (verbose)
			{
				if (result)
					printf("OK\n");
				else
					printf("FAIL\n");
			}
		}
		else
		{
			vars.selectBestCluster();
		}
	}


	RecognitionResult recognizeImage(bool verbose, imago::Settings& vars, const imago::Image& src, const std::string& config)
	{
		std::vector<RecognitionResult> results;
		
		imago::ChemicalStructureRecognizer _csr;
		imago::Molecule mol;

		for (int iter = 0; ; iter++)
		{
			bool good = false;

			vars.general.StartTime = 0;

			try
			{
				imago::Image img;
				img.copy(src);

				if (iter == 0)
				{
					if (!imago::prefilterEntrypoint(vars, img))
						break;
				}
				else
				{
					if (!imago::applyNextPrefilter(vars, img))
						break;
				}

				applyConfig(verbose, vars, config);
				_csr.image2mol(vars, img, mol);

				RecognitionResult result;
				result.molecule = imago::expandSuperatoms(vars, mol);
				result.warnings = mol.getWarningsCount() + mol.getDissolvingsCount() / vars.main.DissolvingsFactor;
				results.push_back(result);

				good = result.warnings <= vars.main.WarningsRecalcTreshold;
			
				if (verbose)
					printf("Filter [%u] done, warnings: %u, good: %u.\n", vars.general.FilterIndex, result.warnings, good);
			}
			catch (std::exception &e)
			{
				if (verbose)
					printf("Filter [%u] exception '%s'.\n", vars.general.FilterIndex, e.what());

		#ifdef _DEBUG
				throw;
		#endif
			}

			if (good)
				break;
		} // for

		RecognitionResult result;
		result.warnings = 999; // just big number to override
		// select the best one
		for (size_t u = 0; u < results.size(); u++)
		{
			if (results[u].warnings < result.warnings)
			{
				result = results[u];
			}
		}
		return result;
	}


	int performFileAction(bool verbose, imago::Settings& vars, const std::string& imageName, const std::string& configName,
						  const std::string& outputName)
	{
		int result = 0; // ok mark
		imago::VirtualFS vfs;

		if (vars.general.ExtractCharactersOnly)
		{
			if (verbose)
				printf("Characters extraction from image '%s'\n", imageName.c_str());
		}
		else
		{
			if (verbose)
				printf("Recognition of image '%s'\n", imageName.c_str());
		}

		try
		{
			imago::Image image;	  

			if (vars.general.LogVFSEnabled)
			{
				imago::getLogExt().SetVirtualFS(vfs);
			}

			imago::ImageUtils::loadImageFromFile(image, imageName.c_str());

			if (vars.general.ExtractCharactersOnly)
			{
				imago::prefilterEntrypoint(vars, image);
				applyConfig(verbose, vars, configName);
				imago::ChemicalStructureRecognizer _csr;
				_csr.extractCharacters(vars, image);
			}
			else
			{
				RecognitionResult result = recognizeImage(verbose, vars, image, configName);		
				imago::FileOutput fout(outputName.c_str());
				fout.writeString(result.molecule.c_str());
			}

		}
		catch (std::exception &e)
		{
			result = 2; // error mark
			puts(e.what());
	#ifdef _DEBUG
			throw;
	#endif
		}

		dumpVFS(vfs, "log_vfs.txt");

		return result;
	}
}