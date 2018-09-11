
#ifndef INCLUDE_26241_H_
#define INCLUDE_26241_H_

#include "lwm2m/objects.h"
namespace KnownObjects {
namespace id26241 {

class instance : public Lwm2mObjectInstance {
public:

	int32_t slave_address;

	int32_t frequency;

	Executable transaction;

	int32_t length;

	Opaque<100> buffer;

	int32_t mode;

};

enum class RESID {

	slave_address = 1,
	frequency,
	transaction,
	length,
	buffer,
	mode,
};

class object : public Lwm2mObject<26241, object, instance> {
public:

    Resource(1, &instance::slave_address, O_RES_RW) slave_address;
    Resource(2, &instance::frequency, O_RES_RW) frequency;
    Resource(3, &instance::transaction, O_RES_E) transaction;
    Resource(4, &instance::length, O_RES_RW) length;
    Resource(5, &instance::buffer, O_RES_RW) buffer;
    Resource(6, &instance::mode, O_RES_RW) mode;

};

} // end of id namespace
} // end of KnownObjects namespace
inline bool operator== (KnownObjects::id26241::RESID c1, uint16_t c2) { return (uint16_t) c1 == c2; }
inline bool operator== (uint16_t c2, KnownObjects::id26241::RESID c1) { return (uint16_t) c1 == c2; }
#endif /* INCLUDE_26241_H_ */
