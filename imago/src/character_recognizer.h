#ifndef _character_recognizer_h
#define _character_recognizer_h

#include <string>
#include <vector>
#include <map>
#include "character_recognizer_data.h"
#include "symbol_features.h"
#include "segment.h"
#include "stl_fwd.h"
#include "recognition_distance.h"
#include "segment_tools.h"
#include "constants.h"

namespace imago
{
   class Segment;

   bool isPossibleCharacter(const Settings& vars, const Segment& seg, bool loose_cmp = false);
   double getDistanceCapital(const Settings& vars, const Segment& seg);
	
   class CharacterRecognizer: CharacterRecognizerData
   {
   public:
      CharacterRecognizer( int k );
      CharacterRecognizer( int k, const std::string &filename );

      RecognitionDistance recognize(const Settings& vars, const SymbolFeatures &features,
                                     const std::string &candidates, bool wide_range = false ) const;

      RecognitionDistance recognize_all(const Settings& vars, const Segment &seg, 
		                                 const std::string &candidates = all,
										 bool can_adjust = true) const;

	  char recognize(const Settings& vars, const Segment &seg, const std::string &candidates,
                      double *distance = 0 ) const;

      inline int getDescriptorsCount() const {return _count;}
      ~CharacterRecognizer();

      static const std::string upper, lower, digits, all;
	  static const std::string like_bonds;

      static double _compareFeatures(const Settings& vars,  const SymbolFeatures &f1,
                                      const SymbolFeatures &f2 );
   private:
      void _loadData( std::istream &in );
      static double _compareDescriptors(const Settings& vars,  const std::vector<double> &d1,
                                         const std::vector<double> &d2 );      
      int _k;
   };

   class HWCharacterRecognizer
   {
   public:
      SymbolFeatures features_h1;
      SymbolFeatures features_h2; 
      SymbolFeatures features_h3;
      SymbolFeatures features_h4;
      SymbolFeatures features_h5;
      SymbolFeatures features_h6;
      SymbolFeatures features_h7;
      SymbolFeatures features_h8;
      SymbolFeatures features_h9;
      SymbolFeatures features_h10;
      SymbolFeatures features_h11;
            
      SymbolFeatures features_n1;
      SymbolFeatures features_n2;
      SymbolFeatures features_n3;
      SymbolFeatures features_n4;
      SymbolFeatures features_n5;
      SymbolFeatures features_n6;
      HWCharacterRecognizer ( const CharacterRecognizer &cr );

      int recognize (const Settings& vars, Segment &seg);
   protected:
      const CharacterRecognizer &_cr;
      void _readFile(const char *filename, SymbolFeatures &features);
};

}

#endif /* _character_recognizer_h */
