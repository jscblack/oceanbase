/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX SHARE_SCHEMA
#include "ob_inner_table_schema.h"

#include "share/schema/ob_schema_macro_define.h"
#include "share/schema/ob_schema_service_sql_impl.h"
#include "share/schema/ob_table_schema.h"
#include "share/scn.h"

namespace oceanbase
{
using namespace share::schema;
using namespace common;
namespace share
{

int ObInnerTableSchema::all_virtual_query_response_time_ora_schema(ObTableSchema &table_schema)
{
  int ret = OB_SUCCESS;
  uint64_t column_id = OB_APP_MIN_COLUMN_ID - 1;

  //generated fields:
  table_schema.set_tenant_id(OB_SYS_TENANT_ID);
  table_schema.set_tablegroup_id(OB_INVALID_ID);
  table_schema.set_database_id(OB_ORA_SYS_DATABASE_ID);
  table_schema.set_table_id(OB_ALL_VIRTUAL_QUERY_RESPONSE_TIME_ORA_TID);
  table_schema.set_rowkey_split_pos(0);
  table_schema.set_is_use_bloomfilter(false);
  table_schema.set_progressive_merge_num(0);
  table_schema.set_rowkey_column_num(0);
  table_schema.set_load_type(TABLE_LOAD_TYPE_IN_DISK);
  table_schema.set_table_type(VIRTUAL_TABLE);
  table_schema.set_index_type(INDEX_TYPE_IS_NOT);
  table_schema.set_def_type(TABLE_DEF_TYPE_INTERNAL);

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_table_name(OB_ALL_VIRTUAL_QUERY_RESPONSE_TIME_ORA_TNAME))) {
      LOG_ERROR("fail to set table_name", K(ret));
    }
  }

  if (OB_SUCC(ret)) {
    if (OB_FAIL(table_schema.set_compress_func_name(OB_DEFAULT_COMPRESS_FUNC_NAME))) {
      LOG_ERROR("fail to set compress_func_name", K(ret));
    }
  }
  table_schema.set_part_level(PARTITION_LEVEL_ZERO);
  table_schema.set_charset_type(ObCharset::get_default_charset());
  table_schema.set_collation_type(ObCollationType::CS_TYPE_UTF8MB4_BIN);

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("TENANT_ID", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObNumberType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      38, //column_length
      38, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("SVR_IP", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      1, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_UTF8MB4_BIN, //column_collation_type
      MAX_IP_ADDR_LENGTH, //column_length
      2, //column_precision
      -1, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("SVR_PORT", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      2, //part_key_pos
      ObNumberType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      38, //column_length
      38, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("RESPONSE_TIME", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObNumberType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      38, //column_length
      38, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("COUNT", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObNumberType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      38, //column_length
      38, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("TOTAL", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObNumberType, //column_type
      CS_TYPE_INVALID, //column_collation_type
      38, //column_length
      38, //column_precision
      0, //column_scale
      false, //is_nullable
      false); //is_autoincrement
  }

  if (OB_SUCC(ret)) {
    ADD_COLUMN_SCHEMA("SQL_TYPE", //column_name
      ++column_id, //column_id
      0, //rowkey_id
      0, //index_id
      0, //part_key_pos
      ObVarcharType, //column_type
      CS_TYPE_UTF8MB4_BIN, //column_collation_type
      128, //column_length
      2, //column_precision
      -1, //column_scale
      true, //is_nullable
      false); //is_autoincrement
  }
  if (OB_SUCC(ret)) {
    table_schema.get_part_option().set_part_num(1);
    table_schema.set_part_level(PARTITION_LEVEL_ONE);
    table_schema.get_part_option().set_part_func_type(PARTITION_FUNC_TYPE_LIST);
    if (OB_FAIL(table_schema.get_part_option().set_part_expr("SVR_IP, SVR_PORT"))) {
      LOG_WARN("set_part_expr failed", K(ret));
    } else if (OB_FAIL(table_schema.mock_list_partition_array())) {
      LOG_WARN("mock list partition array failed", K(ret));
    }
  }
  table_schema.set_index_using_type(USING_HASH);
  table_schema.set_row_store_type(ENCODING_ROW_STORE);
  table_schema.set_store_format(OB_STORE_FORMAT_DYNAMIC_MYSQL);
  table_schema.set_progressive_merge_round(1);
  table_schema.set_storage_format_version(3);
  table_schema.set_tablet_id(0);

  table_schema.set_max_used_column_id(column_id);
  return ret;
}


} // end namespace share
} // end namespace oceanbase
