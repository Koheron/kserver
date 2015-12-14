/// Spectrum analyzer driver
///
/// (c) Koheron

#ifndef __DRIVERS_CORE_SPECTRUM_HPP__
#define __DRIVERS_CORE_SPECTRUM_HPP__

#include "dev_mem.hpp"
#include "wr_register.hpp"
#include "addresses.hpp"

#include <signal/kvector.hpp>
 
#define MAP_SIZE 4096
#define SAMPLING_RATE 125E6

//> \description Spectrum analyzer driver
class Spectrum
{
  public:
    Spectrum(Klib::DevMem& dev_mem_);
    ~Spectrum();

    //> \description Open the device
    //> \io_type WRITE
    //> \status ERROR_IF_NEG
    //> \on_error Cannot open SPECTRUM device
    //> \flag AT_INIT
    int Open(uint32_t samples_num_);
    
    void Close();
    
    //> \io_type WRITE
    void set_scale_sch(uint32_t scale_sch);
    
    //> \io_type WRITE
    void set_offset(uint32_t offset_real, uint32_t offset_imag);

    //> \description Read the acquired data
    //> \io_type READ
    Klib::KVector<float>& get_spectrum();
    
    //> \description Number of averages
    //> \io_type READ
    uint32_t get_num_average();

    enum Status {
        CLOSED,
        OPENED,
        FAILED
    };

    //> \is_failed
    bool IsFailed() const {return status == FAILED;}

  private:
    Klib::DevMem& dev_mem;

    int status;
    
    uint32_t samples_num;
    uint32_t acq_time_us;

    // Memory maps IDs:
    Klib::MemMapID config_map;
    Klib::MemMapID status_map;
    Klib::MemMapID spectrum_map;
    Klib::MemMapID demod_map;
    
    // Acquired data buffers
    Klib::KVector<float> data;
    
    // Internal functions
    void _wait_for_acquisition();
    void _raw_to_vector(uint32_t *raw_data);
}; // class Spectrum

#endif // __DRIVERS_CORE_SPECTRUM_HPP__
