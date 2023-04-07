#ifndef __GEOMETRY_ROUTINES
#define __GEOMETRY_ROUTINES

#include "core_debug.h"
#include "geometry_structures.h"
#include "mvg_decomposition.h"

// decompose projective matrix into rotation, translation and internal calibration matrix 
// note check this
bool geometry_calibration_from_P(const size_t shot_id);

// assemble projective matrix from rotation, translation and internal calibration matrix 
void geometry_calibration_from_decomposed_matrices(const size_t shot_id);

// lattice test 
// todo check for points out of picture
bool geometry_lattice_test(const size_t shot_id);

#endif
