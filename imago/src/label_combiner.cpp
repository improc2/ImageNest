/****************************************************************************
 * Copyright (C) 2009-2010 GGA Software Services LLC
 * 
 * This file is part of Imago toolkit.
 * 
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 3 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 ***************************************************************************/

#include <algorithm>
#include <vector>
#include <cfloat>

#include "boost/foreach.hpp"
#include "boost/graph/connected_components.hpp"

#include "label_combiner.h"
#include "log_ext.h"
#include "character_recognizer.h"
#include "rng_builder.h"
#include "segments_graph.h"
#include "segment_tools.h"
#include "image_utils.h"
#include "image_draw_utils.h"
#include "output.h"
#include "algebra.h"

using namespace imago;

LabelCombiner::LabelCombiner(Settings& vars, SegmentDeque &symbols_layer, SegmentDeque &other_layer, const CharacterRecognizer &cr ) :
   _symbols_layer(symbols_layer), _cr(cr), _graphic_layer(other_layer)
   
{
	if (vars.dynamic.CapitalHeight > 0.0)
	{
		_labels.clear();
		_locateLabels(vars);
		BOOST_FOREACH(Label &l, _labels)
		{
			_fillLabelInfo(vars, l);
		}
	}
}

void LabelCombiner::extractLabels( std::deque<Label> &labels )
{
   labels.assign(_labels.begin(), _labels.end());
}

void LabelCombiner::_locateLabels(const Settings& vars)
{
	logEnterFunction();

   using namespace segments_graph;

   SegmentsGraph seg_graph;
   add_segment_range(_symbols_layer.begin(), _symbols_layer.end(), seg_graph);

   RNGBuilder::build(seg_graph);
   
   VertexPosMap::type positions = get(boost::vertex_pos, seg_graph);
   SegmentsGraph::edge_iterator ei, ei_end, next;
   boost::tie(ei, ei_end) = boost::edges(seg_graph);

   double distance_constraint = vars.dynamic.CapitalHeight * vars.lcomb.MaximalDistanceFactor;
   double distance_constraint_y = vars.dynamic.CapitalHeight * vars.lcomb.MaximalYDistanceFactor;

   boost::property_map<segments_graph::SegmentsGraph, boost::vertex_seg_ptr_t>::
	   type img_ptrs = boost::get(boost::vertex_seg_ptr, seg_graph);

   
   for (next = ei; ei != ei_end; ei = next)
   {
	  Segment *s_b = boost::get(img_ptrs, boost::vertex(boost::source(*ei, seg_graph), seg_graph));
	  Segment *s_e = boost::get(img_ptrs, boost::vertex(boost::target(*ei, seg_graph), seg_graph));
      
      ++next;
	  if (SegmentTools::getRealDistance(*s_b,*s_e, SegmentTools::dtEuclidian) < distance_constraint &&
		  SegmentTools::getRealDistance(*s_b,*s_e, SegmentTools::dtDeltaY) < distance_constraint_y )
		  continue;
	  else
         boost::remove_edge(*ei, seg_graph);
   }
   
   getLogExt().appendGraph(vars, "seg_graph", seg_graph);

   std::vector<int> _components(boost::num_vertices(seg_graph));
   int cc = boost::connected_components(seg_graph, &_components[0]);
   std::vector<std::vector<int> > components(cc);
   for (size_t i = 0; i < _components.size(); i++)
      components[_components[i]].push_back(i);

   boost::property_map<segments_graph::SegmentsGraph, boost::vertex_seg_ptr_t>::
                   type seg_ptrs = boost::get(boost::vertex_seg_ptr, seg_graph);
   _labels.resize(cc);
   for (size_t i = 0; i < components.size(); i++)
   {      
	   getLogExt().append("Component", i);
	   getLogExt().appendVector("Subcomponents", components[i]);
      _labels[i].symbols.resize(components[i].size());
      for (int j = 0; j < (int)components[i].size(); j++)
      {
         _labels[i].symbols[j] = boost::get(seg_ptrs, boost::vertex(
                                            components[i][j], seg_graph));
      }
   }

}

void LabelCombiner::_fillLabelInfo(const Settings& vars, Label &l )
{
	logEnterFunction();

   std::vector<Segment*> &symbols = l.symbols;
   int size = symbols.size();
   std::sort(symbols.begin(), symbols.end(), _segmentsComparator);

   int first_line_y = -1;
   int new_line_sep = -1;
   for (int i = 0, first_cap = -1; i < size; i++)
   {
	   if (first_cap < 0 && symbols[i]->getHeight() > 
		   vars.estimation.CapitalHeightError * vars.lcomb.FillLabelFactor1 * vars.dynamic.CapitalHeight)
      {
         first_cap = i;
         first_line_y = symbols[i]->getY() + symbols[i]->getHeight();
         continue;
      }
      
      if (first_cap >= 0)
      {
		  int mid = round(symbols[i]->getY() + vars.lcomb.FillLabelFactor2 * symbols[i]->getHeight());

		  if (mid - first_line_y > vars.dynamic.CapitalHeight * vars.estimation.CharactersSpaceCoeff)
         {
            new_line_sep = i;
            break;
         }
      }
   }

   if (new_line_sep > 0)
   {
      const Segment *seg1, *seg2;

      l.rect.y = symbols[0]->getY();
      std::sort(symbols.begin(), symbols.begin() + new_line_sep, _segmentsCompareX);
      l.rect.x = symbols[0]->getX();

      seg1 = symbols[size - 1];
      l.rect.height = seg1->getY() + seg1->getHeight() - l.rect.y;
      std::sort(symbols.begin() + new_line_sep, symbols.end(), _segmentsCompareX);
      
      seg1 = symbols[new_line_sep - 1];
      seg2 = symbols[size - 1];
      l.rect.width = std::max(seg1->getX() + seg1->getWidth(),
                              seg2->getX() + seg2->getWidth()) - l.rect.x;

      l.line_y = first_line_y;
      l.multi_begin = new_line_sep;
      for (int i = new_line_sep; i < size; i++)
      {
		  if (symbols[i]->getHeight() > vars.estimation.CapitalHeightError * vars.dynamic.CapitalHeight)
         {
            l.multi_line_y = symbols[i]->getY() + symbols[i]->getHeight();
            break;
         }
      }
   } 
   else
   {
      const Segment *seg1;
      l.rect.y = symbols[0]->getY();
      seg1 = symbols[size - 1];
      l.rect.height = seg1->getY() + seg1->getHeight() - l.rect.y;

      std::sort(symbols.begin(), symbols.end(), _segmentsCompareX);

      l.rect.x = symbols[0]->getX();
      seg1 = symbols[size - 1];
      l.rect.width = seg1->getX() + seg1->getWidth() - l.rect.x;

      l.line_y = first_line_y;
   }
}

bool LabelCombiner::_segmentsComparator( const Segment* const &a,
                                         const Segment* const &b )
{
   if (a->getY() < b->getY())
      return true;
   else if (a->getY() == b->getY())
   {
      if (a->getX() < b->getX())
         return true;
   }
   
   return false;      
}

bool LabelCombiner::_segmentsCompareX( const Segment* const &a,
                                       const Segment* const &b )
{
   if (a->getX() < b->getX())
      return true;

   return false;
}


LabelCombiner::~LabelCombiner()
{
}

