/* Additional abstraction level for I2C sensors acting as I2C remote
 * memory units.
 */

#include "at91sam7s256.h"

#include "mytypes.h"
#include "nxt.h"
#include "sensors.h"
#include "i2c.h"

/** Initializes a remote memory unit of address 'address' on the given
 * sensor port.
 */
void i2c_memory_init(U8 sensor, U8 address)
{
  sensors_i2c_enable(sensor);
  i2c_register(sensor, address);
}

/** Reads a value at 'internal_address' in the memory unit on the
 * given sensor port and returns it in the given buffer. Size is the
 * expected returned data size in bytes. The buffer 'buf' should be
 * pre-allocated by the caller.
 */
void i2c_memory_read(U8 sensor, U8 internal_address, U8 *buf, U8 size)
{

}
