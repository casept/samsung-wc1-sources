#ifdef __TIZEN_PATCH__
/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014  Instituto Nokia de Tecnologia - INdT
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

struct btd_attribute;

void gatt_init(void);

void gatt_cleanup(void);

/*
 * Read operation result callback. Called from the service implementation
 * informing the core (ATT layer) the result of read operation.
 * @err:	error in -errno format.
 * @value:	value of the attribute read.
 * @len:	length of value.
 * @user_data:	user_data passed in btd_attr_read_t callback.
 */
#ifdef __TIZEN_PATCH__
typedef uint8_t (*btd_attr_read_result_t) (int err, uint8_t *value, size_t len,
							void *user_data);
#else
typedef void (*btd_attr_read_result_t) (int err, uint8_t *value, size_t len,
							void *user_data);
#endif

/*
 * Service implementation callback passed to core (ATT layer). It manages read
 * operations received from remote devices.
 * @attr:	reference of the attribute to be read.
 * @bdaddr: Remote device address, which requets read value
 * @result:	callback called from the service implementation informing the
 *		value of attribute read.
 * @user_data:	user_data passed in btd_attr_read_result_t callback.
 */
#ifdef __TIZEN_PATCH__
typedef uint8_t (*btd_attr_read_t) (struct btd_attribute *attr, bdaddr_t *bdaddr,
						btd_attr_read_result_t result,
						void *user_data);
#else
typedef void (*btd_attr_read_t) (struct btd_attribute *attr,
						btd_attr_read_result_t result,
						void *user_data);
#endif

/*
 * Write operation result callback. Called from the service implementation
 * informing the core (ATT layer) the result of the write operation. It is used
 * to manage Write Request operations.
 * @err:	error in -errno format.
 * @user_data:	user_data passed in btd_attr_write_t callback.
 */
#ifdef __TIZEN_PATCH__
typedef uint8_t (*btd_attr_write_result_t) (int err, 	void *user_data);
#else
typedef void (*btd_attr_write_result_t) (int err, void *user_data);
#endif
/*
 * Service implementation callback passed to core (ATT layer). It manages write
 * operations received from remote devices.
 * @attr:	reference of the attribute to be changed.
 * @value:	new attribute value.
 * @len:	length of value.
 * @result:	callback called from the service implementation informing the
 *		result of the write operation.
 * @user_data:	user_data passed in btd_attr_write_result_t callback.
 */
#ifdef __TIZEN_PATCH__
typedef uint8_t (*btd_attr_write_t) (struct btd_attribute *attr,
					const uint8_t *value, size_t len,
					btd_attr_write_result_t result,
					void *user_data);
#else
typedef void (*btd_attr_write_t) (struct btd_attribute *attr,
					const uint8_t *value, size_t len,
					btd_attr_write_result_t result,
					void *user_data);
#endif

/* btd_gatt_add_service - Add a service declaration to local attribute database.
 * @uuid:	Service UUID.
 * @path: 	Service object path.
 *
 * Returns a reference to service declaration attribute. In case of error,
 * NULL is returned.
 */
#ifdef __TIZEN_PATCH__
struct btd_attribute *btd_gatt_add_service(const bt_uuid_t *uuid, const char *path);
#else
struct btd_attribute *btd_gatt_add_service(const bt_uuid_t *uuid);
#endif

#ifdef __TIZEN_PATCH__
/*
 * btd_gatt_update_attr_db - updates the attribute data base.
 */
bool btd_gatt_update_attr_db(void);

/*
 * btd_get_service_path - Gets the Service object path if registerd.
 */
char *btd_get_service_path(bt_uuid_t uuid);

/*
 * btd_get_attrib_path - Gets the attribute path if registerd.
 */
const char *btd_get_attrib_path(struct btd_attribute *attrib);

/*
 * btd_find_service_from_attr - Gets the Service from attribute if registerd.
 */
const char *btd_find_service_from_attr(struct btd_attribute *attrib);

/*
 * service_append_dict - Prepare the dictionar entry for a Service
 * along with characteristics and discriptors.
 */
DBusMessage *service_append_dict(bt_uuid_t uuid, DBusMessage *msg);

/*
 * btd_gatt_set_notify_indicate_flag - Set the notification and indication flag.
 * @attrib:	attribute.
 * @notify_indicate: flag to set
 */
void btd_gatt_set_notify_indicate_flag(struct btd_attribute *attrib,
						bool notify_indicate);
#endif

/*
 * btd_gatt_remove_service - Remove a service (along with all its
 * characteristics) from the local attribute database.
 * @service:	Service declaration attribute.
 */
void btd_gatt_remove_service(struct btd_attribute *service);

/*
 * btd_gatt_add_char - Add a characteristic (declaration and value attributes)
 * to local attribute database.
 * @uuid:	Characteristic UUID (16-bits or 128-bits).
 * @properties: Characteristic properties. See Core SPEC 4.1 page 2183.
 * @path: 	Characteristic object path.
 * @read_cb:	Callback used to provide the characteristic value.
 * @write_cb:	Callback called to notify the implementation that a new value
 *              is available.
 *
 * Returns a reference to characteristic value attribute. In case of error,
 * NULL is returned.
 */
#ifdef __TIZEN_PATCH__
struct btd_attribute *btd_gatt_add_char(const bt_uuid_t *uuid,
						uint8_t properties,
						const char *path,
						btd_attr_read_t read_cb,
						btd_attr_write_t write_cb);
#else
struct btd_attribute *btd_gatt_add_char(const bt_uuid_t *uuid,
						uint8_t properties,
						btd_attr_read_t read_cb,
						btd_attr_write_t write_cb);
#endif

/*
 * btd_gatt_add_char_desc - Add a characteristic descriptor to the local
 * attribute database.
 * @uuid:	Characteristic Descriptor UUID (16-bits or 128-bits).
 * @path: 	Characteristic object path.
 * @read_cb:	Callback that should be called once the characteristic
 *		descriptor attribute is read.
 * @write_cb:	Callback that should be called once the characteristic
 *		descriptor attribute is written.
 *
 * Returns a reference to characteristic descriptor attribute. In case of
 * error, NULL is returned.
 */
#ifdef __TIZEN_PATCH__
struct btd_attribute *btd_gatt_add_char_desc(const bt_uuid_t *uuid,
						const char *path,
						btd_attr_read_t read_cb,
						btd_attr_write_t write_cb);
#else
struct btd_attribute *btd_gatt_add_char_desc(const bt_uuid_t *uuid,
						btd_attr_read_t read_cb,
						btd_attr_write_t write_cb);
#endif

#ifdef __TIZEN_PATCH__
/*
 * btd_gatt_update_char - update characteristic value to the local
 * attribute database.
 * @uuid:	Characteristic UUID (16-bits or 32-bits or 128-bits).
 * @value:	charateristci value to update the databas with
 * @len:	length of the value.
 *
 * Returns a TRUE or FALSE.
 */
bool btd_gatt_update_char(const bt_uuid_t *uuid, uint8_t *value, size_t len);
#endif

#endif
