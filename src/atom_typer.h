/** \file atom_typer.h
 *
 *  Classes and routines for reducing an atom down to a numerical type or vector.
 *
 *  Created on: Feb 27, 2019
 *      Author: dkoes
 */

#ifndef ATOMTYPER_H_
#define ATOMTYPER_H_

#include <openbabel/atom.h>
#include <openbabel/elements.h>
#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>

namespace libmolgrid {

/***** Base classes **********/

/** \brief Base class for generating numerical types along with atomic radius */
class AtomIndexTyper {
  public:
    AtomIndexTyper();
    virtual ~AtomIndexTyper();

    /// return number of types
    virtual unsigned num_types() const;

    ///return type index of a along with the apprioriate index
    virtual std::pair<int,float> get_type(OpenBabel::OBAtom& a) const = 0;

    //return vector of string representations of types
    //this isn't expected to be particularly efficient
    virtual std::vector<std::string> get_type_names() const = 0;
};

/** \brief Base class for generating vector types */
class AtomVectorTyper {
  public:
    AtomVectorTyper() {}
    virtual ~AtomVectorTyper() {}

    /// return number of types
    virtual unsigned num_types() const = 0;

    ///set vector type of atom a, return radius
    virtual float get_type(OpenBabel::OBAtom& a, std::vector<float>& typ) const = 0;

    //return vector of string representations of types
    //this isn't expected to be particularly efficient
    virtual std::vector<std::string> get_type_names() const = 0;

};

/** \brief Base class for mapping between type indices */
class AtomIndexTypeMapper {
  public:
    AtomIndexTypeMapper() {}
    virtual ~AtomIndexTypeMapper() {}

    /// return number of mapped types, zero if unknown (no mapping)
    virtual unsigned num_types() const { return 0; };

    /// return mapped type
    virtual int get_type(unsigned origt) const { return origt; }

    //return vector of string representations of types
    virtual std::vector<std::string> get_type_names() const = 0;
};


/*********** Atom typers *****************/

/** \brief Calculate gnina types
 *
 * These are variants of AutoDock4 types. */
class GninaIndexTyper: public AtomIndexTyper {
  public:

    enum type {
      /* 0 */Hydrogen, // H_H_X,
      /* 1 */PolarHydrogen,//(can donate) H_HD_X,
      /* 2 */AliphaticCarbonXSHydrophobe,// C_C_C_H,  //hydrophobic according to xscale
      /* 3 */AliphaticCarbonXSNonHydrophobe,//C_C_C_P, //not hydrophobic (according to xs)
      /* 4 */AromaticCarbonXSHydrophobe,//C_A_C_H,
      /* 5 */AromaticCarbonXSNonHydrophobe,//C_A_C_P,
      /* 6 */Nitrogen,//N_N_N_P, no hydrogen bonding
      /* 7 */NitrogenXSDonor,//N_N_N_D,
      /* 8 */NitrogenXSDonorAcceptor,//N_NA_N_DA, also an autodock acceptor
      /* 9 */NitrogenXSAcceptor,//N_NA_N_A, also considered an acceptor by autodock
      /* 10 */Oxygen,//O_O_O_P,
      /* 11 */OxygenXSDonor,//O_O_O_D,
      /* 12 */OxygenXSDonorAcceptor,//O_OA_O_DA, also an autodock acceptor
      /* 13 */OxygenXSAcceptor,//O_OA_O_A, also an autodock acceptor
      /* 14 */Sulfur,//S_S_S_P,
      /* 15 */SulfurAcceptor,//S_SA_S_P, XS doesn't do sulfur acceptors
      /* 16 */Phosphorus,//P_P_P_P,
      /* 17 */Fluorine,//F_F_F_H,
      /* 18 */Chlorine,//Cl_Cl_Cl_H,
      /* 19 */Bromine,//Br_Br_Br_H,
      /* 20 */Iodine,//I_I_I_H,
      /* 21 */Magnesium,//Met_Mg_Met_D,
      /* 22 */Manganese,//Met_Mn_Met_D,
      /* 23 */Zinc,// Met_Zn_Met_D,
      /* 24 */Calcium,//Met_Ca_Met_D,
      /* 25 */Iron,//Met_Fe_Met_D,
      /* 26 */GenericMetal,//Met_METAL_Met_D,
      /* 27 */Boron,//there are 160 cmpds in pdbbind (general, not refined) with boron
      NumTypes
    };

    /** Information for an atom type.  This includes many legacy fields. */
    struct info
    {
      type sm;
      const char* smina_name; //this must be more than 2 chars long
      const char* adname;//this must be no longer than 2 chars
      unsigned anum;
      float ad_radius;
      float ad_depth;
      float ad_solvation;
      float ad_volume;
      float covalent_radius;
      float xs_radius;
      bool xs_hydrophobe;
      bool xs_donor;
      bool xs_acceptor;
      bool ad_heteroatom;
    };

  private:
    bool use_covalent = false;
    static const info default_data[NumTypes];
    const info *data = NULL; //data to use

  public:

    //Create a gnina typer.  If usec is true, use the gnina determined covalent radius.
    GninaIndexTyper(bool usec = false, const info *d = default_data): use_covalent(usec), data(d) {}
    virtual ~GninaIndexTyper() {}

    /// return number of types
    virtual unsigned num_types() const;

    ///return type index of a
    virtual std::pair<int,float> get_type(OpenBabel::OBAtom& a) const;

    //return vector of string representations of types
    virtual std::vector<std::string> get_type_names() const;

    ///return gnina info for a given type
    const info& get_info(int t) const { return data[t]; }
};

/** \brief Calculate element types
 *
 * There are quite a few elements, so should probably run this through
 * an organic chem atom mapper that reduces to number of types.
 * The type id is the atomic number.  Any element with atomic number
 * greater than or equal to the specified max is assigned type zero.
 *  */
class ElementIndexTyper: public AtomIndexTyper {
    unsigned last_elem;
  public:
    ElementIndexTyper(unsigned maxe = 84): last_elem(maxe) {}
    virtual ~ElementIndexTyper() {}

    /// return number of types
    virtual unsigned num_types() const;

    ///return type index of a
    virtual std::pair<int,float> get_type(OpenBabel::OBAtom& a) const;

    //return vector of string representations of types
    virtual std::vector<std::string> get_type_names() const;
};

/** \brief Wrap an atom typer with a mapper
 *
 */
template<class Mapper, class Typer>
class MappedAtomIndexTyper: public AtomIndexTyper {
    Mapper mapper;
    Typer typer;
  public:
    MappedAtomIndexTyper(const Mapper& map, const Typer& typr): mapper(map), typer(typr) {}
    virtual ~MappedAtomIndexTyper() {}

    /// return number of types
    virtual unsigned num_types() const {
      return mapper.num_types();
    }

    ///return type index of a
    virtual std::pair<int,float> get_type(OpenBabel::OBAtom& a) const {
      auto res_rad = typer.get_type(a);
      //remap the type
      int ret = mapper.get_type(res_rad.first);
      return make_pair(ret, res_rad.second);
    }

    //return vector of string representations of types
    virtual std::vector<std::string> get_type_names() const {
      return mapper.get_type_names();
    }
};


/** \brief Decompose gnina types into elements and properties.  Result is boolean.
 *
 * Hydrophobic, Aromatic, Donor, Acceptor
 *
 * These are variants of AutoDock4 types. */
class GninaVectorTyper: public AtomVectorTyper {
    GninaIndexTyper ityper;
    static std::vector<std::string> vtype_names;
  public:
    enum vtype {
      /* 0 */Hydrogen,
      /* 1 */Carbon,
      /* 2 */Nitrogen,
      /* 3 */Oxygen,
      /* 4 */Sulfur,
      /* 5 */Phosphorus,
      /* 6 */Fluorine,
      /* 7 */Chlorine,
      /* 8 */Bromine,
      /* 9 */Iodine,
      /* 10 */Magnesium,
      /* 11 */Manganese,
      /* 12 */Zinc,
      /* 13 */Calcium,
      /* 14 */Iron,
      /* 15 */Boron,
      /* 16 */GenericAtom,
      /* 17 */AD_depth, //floating point
      /* 18 */AD_solvation, //float
      /* 19 */AD_volume, //float
      /* 20 */XS_hydrophobe, //bool
      /* 21 */XS_donor, //bool
      /* 22 */XS_acceptor, //bool
      /* 23 */AD_heteroatom, //bool
      /* 24 */OB_partialcharge, //float
      /* 25 */ NumTypes
    };

    GninaVectorTyper(const GninaIndexTyper& ityp = GninaIndexTyper()): ityper(ityp) {}
    virtual ~GninaVectorTyper() {}

    /// return number of types
    virtual unsigned num_types() const;

    ///return type index of a
    virtual float get_type(OpenBabel::OBAtom& a, std::vector<float>& typ) const;

    //return vector of string representations of types
    virtual std::vector<std::string> get_type_names() const;
};


/*********** Atom mappers *****************/

/** \brief Map atom types based on provided file.
 *
 * Each line for the provided file specifies a single type.
 * Types are specified using type names.
 * This class must be provided the type names properly indexed (should match get_type_names).
 */
class FileAtomMapper : public AtomIndexTypeMapper {
    std::vector<std::string> old_type_names;
    std::vector<int> old_type_to_new_type;
    std::vector<std::string> new_type_names;

    //setup map and new type names, assumes old_type_names is initialized
    void setup(std::istream& in);
  public:

    ///initialize from filename
    FileAtomMapper(std::string& fname, const std::vector<std::string>& type_names);

    ///initialize from stream
    FileAtomMapper(std::istream& in, const std::vector<std::string>& type_names): old_type_names(type_names) {
      setup(in);
    }

    virtual ~FileAtomMapper() {}

    /// return number of mapped types, zero if unknown (no mapping)
    virtual unsigned num_types() const { return new_type_names.size(); }

    /// return mapped type
    virtual int get_type(unsigned origt) const;

    //return mapped type names
    virtual std::vector<std::string> get_type_names() const { return new_type_names; }

};

/** \brief Map atom types onto a provided subset.
 */
class SubsetAtomMapper: public AtomIndexTypeMapper {
    std::unordered_map<int, int> old2new;
    int default_type = -1; // if not in map
    unsigned num_new_types = 0;
  public:
    /// Indices of map are new types, values are the old types,
    /// if include_catchall is true, the last type will be the type
    /// returned for anything not in map (otherwise -1 is returned)
    SubsetAtomMapper(const std::vector<int>& map, bool include_catchall=true);

    ///surjective mapping
    SubsetAtomMapper(const std::vector< std::vector<int> >& map, bool include_catchall=true);

    /// return number of mapped types, zero if unknown (no mapping)
    virtual unsigned num_types() const { return num_new_types; }

    /// return mapped type
    virtual int get_type(unsigned origt) const;
};

} /* namespace libmolgrid */

#endif /* ATOMTYPER_H_ */
