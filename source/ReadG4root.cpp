// C++ includes
#include <iostream>
#include <string>
#include <vector>


#include "Qpix/Random.h"
#include "Qpix/Structures.h"
#include "Qpix/ReadG4root.h"


namespace Qpix
{
    // Setups the branches it expects to read
    void READ_G4_ROOT::set_branch_addresses(TChain * chain)
    {
        chain->SetBranchAddress("run", &run_);
        chain->SetBranchAddress("event", &event_);
        chain->SetBranchAddress("number_particles", &number_particles_);
        chain->SetBranchAddress("number_hits", &number_hits_);
        chain->SetBranchAddress("energy_deposit", &energy_deposit_);

        chain->SetBranchAddress("particle_track_id", &particle_track_id_);
        chain->SetBranchAddress("particle_parent_track_id", &particle_parent_track_id_);
        chain->SetBranchAddress("particle_pdg_code", &particle_pdg_code_);
        chain->SetBranchAddress("particle_mass", &particle_mass_);
        chain->SetBranchAddress("particle_charge", &particle_charge_);
        chain->SetBranchAddress("particle_process_key", &particle_process_key_);
        chain->SetBranchAddress("particle_total_occupancy", &particle_total_occupancy_);

        chain->SetBranchAddress("particle_initial_x", &particle_initial_x_);
        chain->SetBranchAddress("particle_initial_y", &particle_initial_y_);
        chain->SetBranchAddress("particle_initial_z", &particle_initial_z_);
        chain->SetBranchAddress("particle_initial_t", &particle_initial_t_);

        chain->SetBranchAddress("particle_initial_px", &particle_initial_px_);
        chain->SetBranchAddress("particle_initial_py", &particle_initial_py_);
        chain->SetBranchAddress("particle_initial_pz", &particle_initial_pz_);
        chain->SetBranchAddress("particle_initial_energy", &particle_initial_energy_);

        chain->SetBranchAddress("hit_track_id", &hit_track_id_);

        chain->SetBranchAddress("hit_start_x", &hit_start_x_);
        chain->SetBranchAddress("hit_start_y", &hit_start_y_);
        chain->SetBranchAddress("hit_start_z", &hit_start_z_);
        chain->SetBranchAddress("hit_start_t", &hit_start_t_);

        chain->SetBranchAddress("hit_end_x", &hit_end_x_);
        chain->SetBranchAddress("hit_end_y", &hit_end_y_);
        chain->SetBranchAddress("hit_end_z", &hit_end_z_);
        chain->SetBranchAddress("hit_end_t", &hit_end_t_);

        chain->SetBranchAddress("hit_energy_deposit", &hit_energy_deposit_);
        chain->SetBranchAddress("hit_length", &hit_length_);
        chain->SetBranchAddress("hit_process_key", &hit_process_key_);
    } // set_branch_addresses()

    // Opens the file and tell you how many events are there
    void READ_G4_ROOT::Open_File( std::string file_ , int& number_entries)
    {          
        // add files to chain
        chain_->Add(file_.c_str());
        // set branch addresses
        set_branch_addresses(chain_);
        
        // get number of entries
        // size_t const number_entries = chain_->GetEntries();
        number_entries = (int)(chain_->GetEntries());

        std::cout << "There are a total of "
                    << number_entries
                    << " events in the sample.\n"
                    << std::endl;
    }//Open_File


    // gets the event from the file and tunrs it into electrons
    void READ_G4_ROOT::Get_Event(int EVENT, Qpix::Qpix_Paramaters * Qpix_params, std::vector<Qpix::ELECTRON>& hit_e)
    {
        chain_->GetEntry(EVENT);

        int indexer = 0;
        // loop over all hits in the event
        for (int h_idx = 0; h_idx < number_hits_; ++h_idx)
        {
            // from PreStepPoint
            double const start_x = hit_start_x_->at(h_idx);  // cm
            double const start_y = hit_start_y_->at(h_idx);  // cm
            double const start_z = hit_start_z_->at(h_idx);  // cm
            double const start_t = hit_start_t_->at(h_idx);  // ns

            // from PostStepPoint
            double const end_x = hit_end_x_->at(h_idx);  // cm
            double const end_y = hit_end_y_->at(h_idx);  // cm
            double const end_z = hit_end_z_->at(h_idx);  // cm
            double const end_t = hit_end_t_->at(h_idx);  // ns

            // energy deposit
            double const energy_deposit = hit_energy_deposit_->at(h_idx);  // MeV

            // calcualte the number of electrons in the hit
            int Nelectron = round(energy_deposit*1e6/Qpix_params->Wvalue);
            // if not enough move on
            if (Nelectron == 0){continue;}

            // define the electrons start position
            double electron_loc_x = start_x +50;
            double electron_loc_y = start_y +50;
            double electron_loc_z = start_z +250;
            double electron_loc_t = start_t;
            // Determin the "step" size
            double const step_x = (end_x - start_x) / Nelectron;
            double const step_y = (end_y - start_y) / Nelectron;
            double const step_z = (end_z - start_z) / Nelectron;
            double const step_t = (end_t - start_t) / Nelectron;

            double electron_x, electron_y, electron_z;
            double T_drift, sigma_L, sigma_T;

            // Loop through the electrons 
            for (int i = 0; i < Nelectron; i++) 
            {
                // calculate frift time for diffusion 
                T_drift = electron_loc_z / Qpix_params->E_vel;
                // electron lifetime
                if (Qpix::RandomUniform() >= exp(-T_drift/Qpix_params->Life_Time)){continue;}
                
                // diffuse the electrons position
                sigma_T = sqrt(2*Qpix_params->DiffusionT*T_drift);
                sigma_L = sqrt(2*Qpix_params->DiffusionL*T_drift);
                electron_x = Qpix::RandomNormal(electron_loc_x,sigma_T);
                electron_y = Qpix::RandomNormal(electron_loc_y,sigma_T);
                electron_z = Qpix::RandomNormal(electron_loc_z,sigma_L);

                // check event is contained after diffused ( one pixel from the edge)
                if (!(0.4 <= electron_x && electron_x <= 99.6) || !(0.4 <= electron_y && electron_y <= 99.6) || !(0.1 <= electron_z && electron_z <= 499.6)){continue;}

                // add the electron to the vector.
                hit_e.push_back(Qpix::ELECTRON());
                
                // convert the electrons x,y to a pixel index
                int Pix_Xloc, Pix_Yloc;
                Pix_Xloc = (int) ceil(electron_x / Qpix_params->Pix_Size);
                Pix_Yloc = (int) ceil(electron_y / Qpix_params->Pix_Size);

                hit_e[indexer].Pix_ID = (int)(Pix_Xloc*10000+Pix_Yloc);
                hit_e[indexer].time = (int)ceil( electron_loc_t + ( electron_z / Qpix_params->E_vel ) ) ;
                
                // Move to the next electron
                electron_loc_x += step_x;
                electron_loc_y += step_y;
                electron_loc_z += step_z;
                electron_loc_t += step_t;
                indexer += 1;
            }

        }

        // sorts the electrons in terms of the pixel ID
        sort(hit_e.begin(), hit_e.end(), Qpix::Electron_Pix_Sort);

    }//Get_Event

}