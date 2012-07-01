#include <mc.h>

int read_pqr_box ( system_t * system ) {

	char buffer[MAXLINE], token[7][MAXLINE];
	int basis_set[3]; 
	FILE * fp;

	printf("INPUT: (read_pqr_box) checking input pqr for basis info\n");

	//flags to make sure we set all basis vectors
	basis_set[0]=basis_set[1]=basis_set[2]=0; 

	/* open the molecule input file */
	fp = fopen(system->pqr_input, "r");
	filecheck(fp,system->pqr_input,READ);	
	
	rewind(fp);
	while ( fgets(buffer, MAXLINE, fp) != NULL ) {
		sscanf(buffer, "%s %s %s %s %s %s %s", 
			token[0], token[1], token[2], token[3], token[4], token[5], token[6]);
		if ( (!strcmp(token[0],"REMARK")) && (!strcmp(token[1],"BOX")) && (!strcmp(token[3],"=")) ) {
			if (!strcmp(token[2],"BASIS[0]")) { //set basis[0]
				{ if (safe_atof(token[4],&(system->pbc->basis[0][0]))) continue; } //make sure each conversion is successful
				{ if (safe_atof(token[5],&(system->pbc->basis[0][1]))) continue; }
				{ if (safe_atof(token[6],&(system->pbc->basis[0][2]))) continue; }
				//if we get this far, then we've successfully read in the basis vector
				basis_set[0] = 1;
			}
			if (!strcmp(token[2],"BASIS[1]")) { //set basis[0]
				{ if (safe_atof(token[4],&(system->pbc->basis[1][0]))) continue; } //make sure each conversion is successful
				{ if (safe_atof(token[5],&(system->pbc->basis[1][1]))) continue; }
				{ if (safe_atof(token[6],&(system->pbc->basis[1][2]))) continue; }
				//if we get this far, then we've successfully read in the basis vector
				basis_set[1] = 1;
			}
			if (!strcmp(token[2],"BASIS[2]")) { //set basis[0]
				{ if (safe_atof(token[4],&(system->pbc->basis[2][0]))) continue; } //make sure each conversion is successful
				{ if (safe_atof(token[5],&(system->pbc->basis[2][1]))) continue; }
				{ if (safe_atof(token[6],&(system->pbc->basis[2][2]))) continue; }
				//if we get this far, then we've successfully read in the basis vector
				basis_set[2] = 1;
			}
			else continue;
		}
		else continue;
	}

	if (basis_set[0] == 1)
		printf("INPUT: basis[0] successfully read from pqr {%.5lf %.5lf %.5lf}\n", 
			system->pbc->basis[0][0], system->pbc->basis[0][1], system->pbc->basis[0][2]);
		else fprintf(stderr,"INPUT: unable to read basis[0] from pqr file.\n");
	if (basis_set[1] == 1)
		printf("INPUT: basis[1] successfully read from pqr {%.5lf %.5lf %.5lf}\n", 
			system->pbc->basis[1][0], system->pbc->basis[1][1], system->pbc->basis[1][2]);
		else fprintf(stderr,"INPUT: unable to read basis[1] from pqr file.\n");
	if (basis_set[2] == 1)
		printf("INPUT: basis[2] successfully read from pqr {%.5lf %.5lf %.5lf}\n", 
			system->pbc->basis[2][0], system->pbc->basis[2][1], system->pbc->basis[2][2]);
		else fprintf(stderr,"INPUT: unable to read basis[2] from pqr file.\n");

	fclose(fp);
	return 0;
}




molecule_t *read_molecules(system_t *system) {

	int i;
	molecule_t *molecules, *molecule_ptr;
	atom_t     *atom_ptr, *prev_atom_ptr;
	char       linebuf[MAXLINE], *n;
	FILE       *fp;
	char       token_atom[MAXLINE],
                   token_atomid[MAXLINE],
                   token_atomtype[MAXLINE],
                   token_moleculetype[MAXLINE];
	char       token_frozen[MAXLINE],
                   token_moleculeid[MAXLINE],
                   token_x[MAXLINE],
                   token_y[MAXLINE],
                   token_z[MAXLINE];
	char token_mass[MAXLINE], token_charge[MAXLINE], token_alpha[MAXLINE], token_epsilon[MAXLINE], token_sigma[MAXLINE], token_omega[MAXLINE], token_gwp_alpha[MAXLINE];
	int current_frozen, current_adiabatic, current_spectre, current_target;
	int current_moleculeid, current_atomid;
	double current_x, current_y, current_z;
	double current_mass, current_charge, current_alpha, current_epsilon, current_sigma, current_omega, current_gwp_alpha;
	double current_molecule_mass;
        int current_site_neighbor;
	int moveable, spectres, targets;
	int atom_counter;

	/* allocate the start of the list */
	molecules = calloc(1, sizeof(molecule_t));
	memnullcheck(molecules,sizeof(molecule_t), __LINE__-1, __FILE__);
	molecule_ptr = molecules;
	molecule_ptr->id = 1;
	molecule_ptr->atoms = calloc(1, sizeof(atom_t));
	memnullcheck(molecule_ptr->atoms,sizeof(atom_t),__LINE__-1, __FILE__);
	atom_ptr = molecule_ptr->atoms;
	prev_atom_ptr = atom_ptr;


	fp = fopen(system->pqr_input, "r");
	filecheck(fp,system->pqr_input,READ);

	/* clear the linebuffer and read the tokens in */
	atom_counter = 0;
	memset(linebuf, 0, MAXLINE);
	n = fgets(linebuf, MAXLINE, fp);
	while(n) {

		/* clear the tokens */
		memset( token_atom,         0, MAXLINE);
		memset( token_atomid,       0, MAXLINE);
		memset( token_atomtype,     0, MAXLINE);
		memset( token_moleculetype, 0, MAXLINE);
		memset( token_frozen,       0, MAXLINE);
		memset( token_moleculeid,   0, MAXLINE);
		memset( token_x,            0, MAXLINE);
		memset( token_y,            0, MAXLINE);
		memset( token_z,            0, MAXLINE);
		memset( token_mass,         0, MAXLINE);
		memset( token_charge,       0, MAXLINE);
		memset( token_alpha,        0, MAXLINE);
		memset( token_epsilon,      0, MAXLINE);
		memset( token_sigma,        0, MAXLINE);
		memset(token_omega, 0, MAXLINE);
		memset(token_gwp_alpha, 0, MAXLINE);

		/* parse the line */
		sscanf(linebuf, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", token_atom, token_atomid, token_atomtype, token_moleculetype, token_frozen, token_moleculeid, token_x, token_y, token_z, token_mass, token_charge, token_alpha, token_epsilon, token_sigma, token_omega, token_gwp_alpha);

		if(!strcasecmp(token_atom, "ATOM") && strcasecmp(token_moleculetype, "BOX")) {

			current_frozen = 0; current_adiabatic = 0; current_spectre = 0; current_target = 0;
			if(!strcasecmp(token_frozen, "F"))
				current_frozen = 1;
			if(!strcasecmp(token_frozen, "A"))
				current_adiabatic = 1;
			if(!strcasecmp(token_frozen, "S"))
				current_spectre = 1;
			if(!strcasecmp(token_frozen, "T"))
				current_target = 1;

			current_moleculeid    = atoi(token_moleculeid);
			current_atomid        = atoi(token_atomid);
			current_x             = atof(token_x);
			current_y             = atof(token_y);
			current_z             = atof(token_z);
			current_mass          = atof(token_mass);	/* mass in amu */
			current_charge        = atof(token_charge);
			current_charge       *= E2REDUCED;		/* convert charge into reduced units  */
			current_alpha         = atof(token_alpha);
			current_epsilon       = atof(token_epsilon);
			current_sigma         = atof(token_sigma);
			current_omega         = atof(token_omega);
			current_gwp_alpha     = atof(token_gwp_alpha);;
			// Functionality of site_neighbor disabled in favor of omega/gwp_alpha parameters
			// Current behavior is to default to atom 0, typically the center of mass for
			// the molecule.
			current_site_neighbor = 0; //atoi( token_site_neighbor );
                        
			if(current_frozen)
                            current_charge *= system->scale_charge;

			if(molecule_ptr->id != current_moleculeid) {
				molecule_ptr->next = calloc(1, sizeof(molecule_t));
				memnullcheck(molecule_ptr,sizeof(molecule_t),__LINE__-1, __FILE__);
				molecule_ptr = molecule_ptr->next;
				molecule_ptr->atoms = calloc(1, sizeof(atom_t));
				memnullcheck(molecule_ptr->atoms,sizeof(atom_t),__LINE__-1, __FILE__);
				prev_atom_ptr->next = NULL;
				free(atom_ptr);
				atom_ptr = molecule_ptr->atoms;
			}

                        strcpy(molecule_ptr->moleculetype, token_moleculetype);

			molecule_ptr->id        = current_moleculeid;
			molecule_ptr->frozen    = current_frozen;
			molecule_ptr->adiabatic = current_adiabatic;
			molecule_ptr->spectre   = current_spectre;
			molecule_ptr->target    = current_target;
			molecule_ptr->mass     += current_mass;

#ifdef QM_ROTATION
			/* if quantum rot calc. enabled, allocate the necessary structures */
			if(system->quantum_rotation && !molecule_ptr->frozen) {

				molecule_ptr->quantum_rotational_energies = calloc(system->quantum_rotation_level_max, sizeof(double));
				memnullcheck(molecule_ptr->quantum_rotational_energies,system->quantum_rotation_level_max*sizeof(double),__LINE__-1, __FILE__);
				molecule_ptr->quantum_rotational_eigenvectors = calloc(system->quantum_rotation_level_max, sizeof(complex_t *));
				memnullcheck(molecule_ptr->quantum_rotational_eigenvectors,system->quantum_rotation_level_max*sizeof(complex_t *),__LINE__-1, __FILE__);
				for(i = 0; i < system->quantum_rotation_level_max; i++){
					molecule_ptr->quantum_rotational_eigenvectors[i] = 
						calloc((system->quantum_rotation_l_max + 1)*(system->quantum_rotation_l_max + 1), sizeof(complex_t));
					memnullcheck(molecule_ptr->quantum_rotational_eigenvectors[i],
						(system->quantum_rotation_l_max+1)*(system->quantum_rotation_l_max+1)*sizeof(complex_t),__LINE__-1, __FILE__);
				}
				molecule_ptr->quantum_rotational_eigensymmetry = calloc(system->quantum_rotation_level_max, sizeof(int));
				memnullcheck(molecule_ptr->quantum_rotational_eigensymmetry,system->quantum_rotation_level_max*sizeof(int),__LINE__-1, __FILE__);

			}
#endif /* QM_ROTATION */

#ifdef XXX
			/* if quantum vib calc. enabled, allocate the necessary structures */
			if(system->quantum_vibration && !molecule_ptr->frozen) {

				molecule_ptr->quantum_vibrational_energies = calloc(system->quantum_vibration_level_max, sizeof(double));
				memnullcheck(molecule_ptr->quantum_vibrational_energies,system->quantum_vibration_level_max*sizeof(double),__LINE__-1, __FILE__);
				molecule_ptr->quantum_vibrational_eigenvectors = calloc(system->quantum_vibration_level_max, sizeof(complex_t *));
				memnullcheck(molecule_ptr->quantum_vibrational_eigenvectors,system->quantum_vibration_level_max*sizeof(complex_t *),__LINE__-1, __FILE__);
				for(i = 0; i < system->quantum_vibration_level_max; i++) {
					molecule_ptr->quantum_vibrational_eigenvectors[i] = 
						calloc((system->quantum_vibration_l_max+1)*(system->quantum_vibration_l_max+1), sizeof(complex_t));
					memnullcheck(molecule_ptr->quantum_vibrational_eigenvectors[i],
						(system->quantum_vibration_l_max+1)*(system->quantum_vibration_l_max+1)*sizeof(complex_t),__LINE__-1, __FILE__);
				}
				molecule_ptr->quantum_vibrational_eigensymmetry = calloc(system->quantum_vibration_level_max, sizeof(int));
				memnullcheck(molecule_ptr->quantum_vibrational_eigensymmetry, system->quantum_vibration_level_max*sizeof(int),__LINE__-1, __FILE__);

			}
#endif /* XXX */

			++atom_counter;
			atom_ptr->id        = atom_counter;
                        atom_ptr->bond_id   = current_atomid;
			memset(atom_ptr->atomtype, 0, MAXLINE);
			strcpy(atom_ptr->atomtype, token_atomtype);
			atom_ptr->frozen    = current_frozen;
			atom_ptr->adiabatic = current_adiabatic;
			atom_ptr->spectre = current_spectre;
			atom_ptr->target = current_target;
			atom_ptr->pos[0] = current_x;
			atom_ptr->pos[1] = current_y;
			atom_ptr->pos[2] = current_z;
			atom_ptr->mass = current_mass;
			atom_ptr->charge = current_charge;
			atom_ptr->polarizability = current_alpha;
			atom_ptr->epsilon = current_epsilon;
			atom_ptr->sigma = current_sigma;
			//if polarvdw is on, we want sigma = 1 or sigma = 0
			//it's an unneeded parameter, complicates the mixing rules and can break LRC
			if ( system->polarvdw ){
				if ( current_epsilon == 0 && current_sigma != 0 ) { //otherwise we break LRC
					fprintf(stderr,"INPUT: site %d has epsilon == 0. setting sigma = 0.\n", atom_ptr->id);
					atom_ptr->sigma=0;
				}
				if ( current_epsilon != 0 && current_sigma != 1 ) { //keeps mixing rules simple
					fprintf(stderr,"INPUT: site has epsilon != 0, but sigma != 1.\n");
					return(NULL);
				}	
			}
			atom_ptr->omega = current_omega;
			atom_ptr->gwp_alpha = current_gwp_alpha;
			if(current_gwp_alpha != 0.)
				atom_ptr->gwp_spin = 1;
			else
				atom_ptr->gwp_spin = 0;

			atom_ptr->site_neighbor_id = current_site_neighbor;
			atom_ptr->next = calloc(1, sizeof(atom_t));
			memnullcheck(atom_ptr->next,sizeof(atom_t),__LINE__-1, __FILE__);
			prev_atom_ptr  = atom_ptr;
			atom_ptr       = atom_ptr->next;
		}

		memset(linebuf, 0, MAXLINE);
		n = fgets(linebuf, MAXLINE, fp);
	}

	/* terminate the atom list */
	prev_atom_ptr->next = NULL;
	free(atom_ptr);

	/* scan the list, make sure that there is at least one moveable molecule */
	moveable = 0;
	spectres = 0;
	targets = 0;
	for(molecule_ptr = molecules; molecule_ptr; molecule_ptr = molecule_ptr->next) {

		if(!molecule_ptr->frozen ) ++moveable;
		if(molecule_ptr ->target ) ++targets;
		if(molecule_ptr ->spectre) ++spectres;

	}

	if(system->spectre) {

		if(!spectres || !targets) {
			error("INPUT: either no targets or spectres found\n");
			return(NULL);
		}

	} else {

		if(!moveable) {
			error("INPUT: no moveable molecules found, there must be at least one in your PQR file\n");
			return(NULL);
		}

	}

	//you forgot to close the file!
	fclose(fp);

	return(molecules);

}



//  read_insertion_molecules( system_t * ) was a cut/paste of read_molecules
//  modified to read the candidate insertion molecules from a separate file. 
///////////////////////////////////////////////////////////////////////////////

// helper functions which will test for presence of a sorbate in 
// the sorbate list and which will add the sorbate if necessary.
int sorbateIs_Not_InList( sorbateAverages_t, char * );
int addSorbateToList( sorbateAverages_t *, char * );

molecule_t *read_insertion_molecules(system_t *system) {

	int i;

	sorbateAverages_t * sorbate;

	molecule_t *molecules, 
	           *molecule_ptr;

	atom_t     *atom_ptr, 
	           *prev_atom_ptr;

	char       linebuf[MAXLINE], *n;

	FILE       *fp;
	char       token_atom[MAXLINE], token_atomid[MAXLINE], token_atomtype[MAXLINE],
	           token_moleculetype[MAXLINE],
	           token_frozen[MAXLINE],
	           token_moleculeid[MAXLINE],
	           token_x[MAXLINE], token_y[MAXLINE], token_z[MAXLINE],
	           token_mass[MAXLINE],
		   token_charge[MAXLINE],
	           token_alpha[MAXLINE], token_epsilon[MAXLINE], token_sigma[MAXLINE], 
	           token_omega[MAXLINE], token_gwp_alpha[MAXLINE];
	
	int        current_frozen, 
	           current_adiabatic,
	           current_spectre, 
	           current_target,
	           current_moleculeid, 
	           current_atomid,
	           current_site_neighbor;
	double     current_x, current_y, current_z,
	           current_mass,  current_charge, 
                   current_alpha, current_epsilon, current_sigma, current_omega, current_gwp_alpha, 
	           current_molecule_mass;

	int        moveable, 
	           spectres, 
	           targets,

	           atom_counter;


	// allocate the start of the list 
	molecules           = calloc(1, sizeof(molecule_t));
	memnullcheck(molecules,sizeof(molecule_t),__LINE__-1, __FILE__);
	molecule_ptr        = molecules;
	molecule_ptr->id    = 1;
	molecule_ptr->atoms = calloc(1, sizeof(atom_t));
	memnullcheck(molecule_ptr->atoms,sizeof(atom_t),__LINE__-1, __FILE__);
	atom_ptr            = molecule_ptr->atoms;
	prev_atom_ptr       = atom_ptr;
	

	// initialize the list of individual sorbate averages
	system->sorbateCount = 0;
	system->sorbateStats.next = NULL;


	// open the molecule input file 
	fp = fopen(system->insert_input, "r");
	filecheck(fp,system->insert_input,READ);

	// clear the linebuffer and read the tokens in 
	atom_counter = 0;
	memset(linebuf, 0, MAXLINE);
	n = fgets(linebuf, MAXLINE, fp);
	while(n) {

		// clear the tokens 
		memset( token_atom,         0, MAXLINE);
		memset( token_atomid,       0, MAXLINE);
		memset( token_atomtype,     0, MAXLINE);
		memset( token_moleculetype, 0, MAXLINE);
		memset( token_frozen,       0, MAXLINE);
		memset( token_moleculeid,   0, MAXLINE);
		memset( token_x,            0, MAXLINE);
		memset( token_y,            0, MAXLINE);
		memset( token_z,            0, MAXLINE);
		memset( token_mass,         0, MAXLINE);
		memset( token_charge,       0, MAXLINE);
		memset( token_alpha,        0, MAXLINE);
		memset( token_epsilon,      0, MAXLINE);
		memset( token_sigma,        0, MAXLINE);
		memset( token_omega,        0, MAXLINE);
		memset( token_gwp_alpha,    0, MAXLINE);

		// parse the input line 
		sscanf(linebuf, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s\n", 
		       token_atom, token_atomid, token_atomtype, 
		       token_moleculetype, 
		       token_frozen, 
		       token_moleculeid, 
		       token_x, token_y, token_z, 
		       token_mass, token_charge, 
		       token_alpha, token_epsilon, token_sigma, token_omega, token_gwp_alpha
		);

		if(!strcasecmp(token_atom, "ATOM") && strcasecmp(token_moleculetype, "BOX")) {

			current_frozen = 0; current_adiabatic = 0; current_spectre = 0; current_target = 0;
			if(!strcasecmp(token_frozen, "F"))
				current_frozen = 1;
			if(!strcasecmp(token_frozen, "A"))
				current_adiabatic = 1;
			if(!strcasecmp(token_frozen, "S"))
				current_spectre = 1;
			if(!strcasecmp(token_frozen, "T"))
				current_target = 1;

			current_moleculeid    = atoi(token_moleculeid);
			current_atomid        = atoi(token_atomid);
			current_x             = atof(token_x);
			current_y             = atof(token_y);
			current_z             = atof(token_z);
			current_mass          = atof(token_mass);	// mass in amu 
			current_charge        = atof(token_charge);
			current_charge       *= E2REDUCED;		// convert charge into reduced units  
			current_alpha         = atof(token_alpha);
			current_epsilon       = atof(token_epsilon);
			current_sigma         = atof(token_sigma);
			current_omega         = atof(token_omega);
			current_gwp_alpha     = atof(token_gwp_alpha);
			// Functionality of site_neighbor disabled in favor of omega/gwp_alpha parameters
			// Current behavior is to default to atom 0, typically the center of mass for
			// the molecule.
			current_site_neighbor = 0;

                        if(current_frozen)
                            current_charge *= system->scale_charge;

			if(molecule_ptr->id != current_moleculeid) {
				molecule_ptr->next = calloc(1, sizeof(molecule_t));
				memnullcheck(molecule_ptr->next,sizeof(molecule_t),__LINE__-1, __FILE__);
				molecule_ptr = molecule_ptr->next;
				molecule_ptr->atoms = calloc(1, sizeof(atom_t));
				memnullcheck(molecule_ptr->atoms,sizeof(atom_t),__LINE__-1, __FILE__);
				prev_atom_ptr->next = NULL;
				free(atom_ptr);
				atom_ptr = molecule_ptr->atoms;
			}
			strcpy(molecule_ptr->moleculetype, token_moleculetype);

			molecule_ptr->id        = current_moleculeid;
			molecule_ptr->frozen    = current_frozen;
			molecule_ptr->adiabatic = current_adiabatic;
			molecule_ptr->spectre   = current_spectre;
			molecule_ptr->target    = current_target;
			molecule_ptr->mass     += current_mass;

#ifdef QM_ROTATION
			/* if quantum rot calc. enabled, allocate the necessary structures */
			if(system->quantum_rotation && !molecule_ptr->frozen) {

				molecule_ptr->quantum_rotational_energies = calloc(system->quantum_rotation_level_max, sizeof(double));
				memnullcheck(molecule_ptr->quantum_rotational_energies, system->quantum_rotation_level_max*sizeof(double),__LINE__-1, __FILE__);
				molecule_ptr->quantum_rotational_eigenvectors = calloc(system->quantum_rotation_level_max, sizeof(complex_t *));
				memnullcheck(molecule_ptr->quantum_rotational_eigenvectors, system->quantum_rotation_level_max*sizeof(complex_t *), __LINE__-1, __FILE__);
				for(i = 0; i < system->quantum_rotation_level_max; i++) {
					molecule_ptr->quantum_rotational_eigenvectors[i] = 
						calloc((system->quantum_rotation_l_max + 1)*(system->quantum_rotation_l_max + 1), sizeof(complex_t));
					memnullcheck(molecule_ptr->quantum_rotational_eigenvectors[i],
						(system->quantum_rotation_l_max + 1)*(system->quantum_rotation_l_max + 1)*sizeof(complex_t),__LINE__-1, __FILE__);
				}
				molecule_ptr->quantum_rotational_eigensymmetry = calloc(system->quantum_rotation_level_max, sizeof(int));
				memnullcheck(molecule_ptr->quantum_rotational_eigensymmetry,system->quantum_rotation_level_max*sizeof(int),__LINE__-1, __FILE__);

			}
#endif /* QM_ROTATION */

#ifdef XXX
			/* if quantum vib calc. enabled, allocate the necessary structures */
			if(system->quantum_vibration && !molecule_ptr->frozen) {

				molecule_ptr->quantum_vibrational_energies = calloc(system->quantum_vibration_level_max, sizeof(double));
				memnullcheck(molecule_ptr->quantum_vibrational_energies, system->quantum_vibration_level_max*sizeof(double), __LINE__-1, __FILE__);
				molecule_ptr->quantum_vibrational_eigenvectors = calloc(system->quantum_vibration_level_max, sizeof(complex_t *));
				memnullcheck(molecule_ptr->quantum_vibrational_eigenvectors, system->quantum_vibration_level_max*sizeof(complex_t *), __LINE__-1, __FILE__);
				for(i = 0; i < system->quantum_vibration_level_max; i++) {
					molecule_ptr->quantum_vibrational_eigenvectors[i] = 
						calloc((system->quantum_vibration_l_max + 1)*(system->quantum_vibration_l_max + 1), sizeof(complex_t));
					memnullcheck(molecule_ptr->quantum_vibrational_eigenvectors[i],
						(system->quantum_vibration_l_max + 1)*(system->quantum_vibration_l_max + 1)*sizeof(complex_t), __LINE__-1, __FILE__);
				}
				molecule_ptr->quantum_vibrational_eigensymmetry = calloc(system->quantum_vibration_level_max, sizeof(int));
				memnullcheck(molecule_ptr->quantum_vibrational_eigensymmetry, system->quantum_vibration_level_max*sizeof(int), __LINE__-1, __FILE__);

			}
#endif /* XXX */

			++atom_counter;
			atom_ptr->id        = atom_counter;
                        atom_ptr->bond_id   = current_atomid;
			memset(atom_ptr->atomtype, 0, MAXLINE);
			strcpy(atom_ptr->atomtype, token_atomtype);
			atom_ptr->frozen    = current_frozen;
			atom_ptr->adiabatic = current_adiabatic;
			atom_ptr->spectre   = current_spectre;
			atom_ptr->target    = current_target;
			atom_ptr->pos[0]    = current_x;
			atom_ptr->pos[1]    = current_y;
			atom_ptr->pos[2]    = current_z;
			atom_ptr->mass      = current_mass;
			atom_ptr->charge    = current_charge;
			atom_ptr->polarizability = current_alpha;
			atom_ptr->epsilon   = current_epsilon;
			atom_ptr->sigma     = current_sigma;
			//if polarvdw is on, we want sigma = 1
			//it's an unneeded parameter, and complicates the mixing rules
			if ( system->polarvdw && current_sigma != 1 ) {
				fprintf(stderr,"INPUT: WARNING POLARVDW REQUIRES SIGMA = 1.\n"
					"INPUT: INPUT VALUE OF %lf IGNORED.\n", current_sigma);
				current_sigma   = 1;
				atom_ptr->sigma = 1;
			}
			atom_ptr->omega     = current_omega;
			atom_ptr->gwp_alpha = current_gwp_alpha;
			if(current_gwp_alpha != 0.)
				atom_ptr->gwp_spin = 1;
			else
				atom_ptr->gwp_spin = 0;

			atom_ptr->site_neighbor_id = current_site_neighbor;
			atom_ptr->next = calloc(1, sizeof(atom_t));
			memnullcheck(atom_ptr->next,sizeof(atom_t),__LINE__-1, __FILE__);
			prev_atom_ptr  = atom_ptr;
			atom_ptr       = atom_ptr->next;
		}

		memset(linebuf, 0, MAXLINE);
		n = fgets(linebuf, MAXLINE, fp);
	}

	// terminate the atom list 
	prev_atom_ptr->next = NULL;
	free(atom_ptr);



	// Count the molecules and create an array of pointers, where each pointer
	// points directly to a molecule in the linked list. While counting the 
	// molecules, check to see whether or not each sorbate is included in the
	// sorbate statistics list, if it is not, we will create an entry for it
	// so that each unique class of sorbate will have its own node in the stats
	// list.
	/////////////////////////////////////////////////////////////////////////////

	// count	
	int molecule_counter = 0;
	for(molecule_ptr = molecules; molecule_ptr; molecule_ptr = molecule_ptr->next) { 
		molecule_counter++;
		// Check to see if this molecule is a new sorbate.
		if( sorbateIs_Not_InList( system->sorbateStats, molecule_ptr->moleculetype ) ) {
			system->sorbateCount++;
			if( !addSorbateToList( &(system->sorbateStats), molecule_ptr->moleculetype )) {
				error( "INPUT: Exhausted memory while constructing sorbate stat list." );
				return NULL;
			}
			for( sorbate = system->sorbateStats.next; sorbate; sorbate = sorbate->next )
				if( !strcasecmp( sorbate->id, molecule_ptr->moleculetype )){
					sorbate->mass = molecule_ptr->mass;
					break;
				}
		}
	}

	// allocate space for array that will give direct access to insertion-candidate
	// molecules in the linked list. 
	system->insertion_molecules_array = malloc( molecule_counter * sizeof(molecule_t*) );
	if(!system->insertion_molecules_array ) {
		error( "INPUT: ERROR - exhausted memory while allocating insertion molecules array.\n" );
		return NULL;
	}

	// point array pointers to their corresponding molecules
	molecule_counter=0;
	for(molecule_ptr = molecules; molecule_ptr; molecule_ptr = molecule_ptr->next) { 
		system->insertion_molecules_array[ molecule_counter ] = molecule_ptr;
		molecule_counter++;
	}
	system->num_insertion_molecules = molecule_counter;


	  //  ERROR CHECKING OF SOME SORT MAY NEED TO GO HERE
	 //   WHAT'S ALLOWED/DISALLOWED FOR INSERTED MOLECULES?
	/////////////////////////////////////////////////////////

	
	return(molecules);
}




// sorbateIs_Not_InList() examines a sorbate stat list and a sorbate ID. 
// Returns true if this sorbate is NOT in the list, false otherwise
int sorbateIs_Not_InList( sorbateAverages_t sorbateList, char *type ) {
	
	sorbateAverages_t *sorbate;

	// Look at each item in the list until we find a match
	// with the sorbate id about which the inquiry was made.
	for( sorbate = sorbateList.next; sorbate; sorbate = sorbate->next ){
		if( !strcasecmp( type, sorbate->id ) )
			// sorbate is already accounted for
			return 0;
	}
	
	// If end of list is reached w/o encountering the sorbate, then
	// the sorbate is new... return 1, i.e. the statement "sorbate 
	// is NOT in list" is TRUE.
	return 1;
}




// addSorbateToList() Takes a sorbate stat list and a sorbate ID. Creates a 
// node at the end of the list and initializes it with the passed sorbate id.
int addSorbateToList( sorbateAverages_t *sorbateList, char *sorbate_type ){
	
	// navigate to the end of the list
	sorbateAverages_t *sorbate = sorbateList;
	while( sorbate->next ){
		sorbate = sorbate->next;
	}

	// allocate a new node for the sorbate
	sorbate->next = malloc( sizeof( sorbateAverages_t ));
	sorbate = sorbate->next;
	
	// return 0 upon failure to allcoate
	if( !sorbate ) return 0;

	// initialize the new node
	strcpy( sorbate->id, sorbate_type );
	sorbate->currentN = 0;
	sorbate->avgN = 0.0;
	//sorbate->N = (double *) calloc( size, sizeof(double) ); // This will be an array w/1 entry per MPI worker.
	sorbate->next = NULL;                                   // Terminate the list.
	
	// return 0 upon failure to allocate
	//if( !sorbate->N ) return 0;

	// send a status update to stdout
	char buffer[MAXLINE];
	sprintf( buffer, "INPUT: System will track individual stats for sorbate %s.\n", sorbate->id );
	output( buffer );

	return 1;
}






#ifdef DEBUG
void test_list(molecule_t *molecules) {

	molecule_t *molecule_ptr;
	atom_t *atom_ptr;
	pair_t *pair_ptr;

	for(molecule_ptr = molecules; molecule_ptr; molecule_ptr = molecule_ptr->next) {

		printf("moleculeid = %d\n", molecule_ptr->id);
		printf("moleculetype = %s\n", molecule_ptr->moleculetype);
		printf("molecule_frozen = %d\n", molecule_ptr->frozen);
		printf("molecule_mass = %f\n", molecule_ptr->mass);
		for(atom_ptr = molecule_ptr->atoms; atom_ptr; atom_ptr = atom_ptr->next) {
			printf("atomtype = %s x = %f y = %f z = %f\n", atom_ptr->atomtype, atom_ptr->pos[0], atom_ptr->pos[1], atom_ptr->pos[2]);
			printf("atom frozen = %d mass = %f, charge = %f, alpha = %f, eps = %f, sig = %f\n", atom_ptr->frozen, atom_ptr->mass, atom_ptr->charge, atom_ptr->polarizability, atom_ptr->epsilon, atom_ptr->sigma);
			for(pair_ptr = atom_ptr->pairs; pair_ptr; pair_ptr = pair_ptr->next)
				if(!(pair_ptr->rd_excluded || pair_ptr->es_excluded || pair_ptr->frozen)) printf("pair = 0x%lx eps = %f sig = %f\n", pair_ptr, pair_ptr->epsilon, pair_ptr->sigma);
		}

	}

	fflush(stdout);

}

void test_molecule(molecule_t *molecule) {

	atom_t *atom_ptr;
	pair_t *pair_ptr;

	printf("moleculeid = %d\n", molecule->id);
	printf("moleculetype = %s\n", molecule->moleculetype);
	printf("molecule_frozen = %d\n", molecule->frozen);
	printf("molecule_mass = %f\n", molecule->mass);
	for(atom_ptr = molecule->atoms; atom_ptr; atom_ptr = atom_ptr->next) {
		printf("atomtype = %s x = %f y = %f z = %f\n", atom_ptr->atomtype, atom_ptr->pos[0], atom_ptr->pos[1], atom_ptr->pos[2]);
		printf("atom frozen = %d mass = %f, charge = %f, alpha = %f, eps = %f, sig = %f\n", atom_ptr->frozen, atom_ptr->mass, atom_ptr->charge, atom_ptr->polarizability, atom_ptr->epsilon, atom_ptr->sigma);
		for(pair_ptr = atom_ptr->pairs; pair_ptr; pair_ptr = pair_ptr->next) {
			printf("pair at 0x%lx\n", pair_ptr);fflush(stdout);
		}
	}

printf("...finished\n");fflush(stdout);

}
#endif /* DEBUG */
