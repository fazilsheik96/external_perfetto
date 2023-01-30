--
-- Copyright 2022 The Android Open Source Project
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     https://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

-- Checks if slice has an ancestor with provided name.
--
-- @arg id INT              Id of the slice to check parents of.
-- @arg parent_name STRING  Name of potential ancestor slice.
-- @ret BOOL                Whether `parent_name` is a name of an ancestor slice.
SELECT
  CREATE_FUNCTION(
    'HAS_PARENT_SLICE_WITH_NAME(id INT, parent_name STRING)',
    'BOOL',
    '
    SELECT EXISTS(
      SELECT 1
      FROM ancestor_slice($id)
      WHERE name = $parent_name
      LIMIT 1
    );
  '
);

-- Count slices with specified name.
--
-- @arg slice_glob STRING Name of the slices to counted.
-- @ret INT               Number of slices with the name.
SELECT CREATE_FUNCTION(
  'SLICE_COUNT(slice_glob STRING)',
  'INT',
  'SELECT COUNT(1) FROM slice WHERE name GLOB $slice_glob'
);