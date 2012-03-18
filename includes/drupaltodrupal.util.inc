<?php
/**
 * @file
 * Utility functions used by the D2D module.
 */

/**
 * Replace all occurrences of the search string with the replacement string
 *
 * This function is just a wrapper for str_replace that also checks if the
 * given string is really is a string. If $string is an integer, it will be
 * converted to a string before replacing. If it's neither a string nor an
 * integer, FALSE is returned.
 *
 * @param mixed $search the value(s) being searched for 
 * @param mixed $replace the replacement value(s)
 * @param string|int $string the value being searched and replaced on
 * @return string|bool the replaced value or FALSE, if replacement failed
 */
function drupaltodrupal_replace($search, $replace, $string) {
  if (is_int($string)) {
    $string = strval($string);
  }
  if (is_string($string)) {
    $string = str_replace($search, $replace, $string); 
  } else {
    return FALSE;
  }
  return $string;
}

/**
 * Implode an array using its key and value pair
 *
 * Returns FALSE if there is a key or value not of type integer or string. The
 * key-value-pair is separated using a '='-character as a separator, several
 * key-value pairs are separated using ','. Moreover, before concatenating the
 * keys and values, the characters '=', ',' and '\\' (single slash) are
 * prepended with an addtional '\\', i.e. they are escaped to allow unique
 * reconstruction of the provided array from the returned string.
 * Note that a unique reconstruction is only possible for array values being of
 * type string, e.g. drupaltodrupal_implode('a=-1') will result in array('a'
 * => '-1') and not in array('a' => -1), that is one might have to convert /
 * cast the key to obtain the desired type.
 *
 * @param array $array the array to be imploded
 * @return string|bool the imploded array or FALSE, if implosion failed
 */
function drupaltodrupal_implode($array) {
  if (!is_array($array)) {
    return FALSE;
  }
  $escaped_array = array();
  foreach ($array as $key => $value) {
    // escape separators and escape-character in $key
    $key = drupaltodrupal_replace(array('\\', ',', '='), array('\\\\', '\\,', '\\='), $key);
    if ($key === FALSE) { // replacement failed
      return FALSE;
    }
    // escape separators and escape-character in $key
    $value = drupaltodrupal_replace(array('\\', ',', '='), array('\\\\', '\\,', '\\='), $value);
    if ($value === FALSE) { // replacement failed
      return FALSE;
    }
    $escaped_array[] = "{$key}={$value}";
  }
  return implode(',', $escaped_array);
}

/**
 * Explode a string into an array of key and value pairs
 *
 * Returns FALSE if the explosion failed, otherwise the array resulting from
 * exploding the given string into key-value-pairs is returned. Note that
 * corresponding keys and values must be separated using '=', while key-value-
 * pairs must be separated using ','. Furthermore, '=' and ',' appearing in
 * keys resp. values must be escaped, see drupaltodrupal_implode for further details.
 *
 * @param string $string the input string
 * @return array|bool the array resulting form splitting the input, FALSE if explosion failed
 */
function drupaltodrupal_explode($string) {
  if (!is_string($string)) {
    return FALSE;
  }
  if (empty($string)) {
    return array();
  }
  // prepend two consecutive slashes by an '@' s.t. every slash followed
  // directly by a ',' resp. '=' really corresponds to an escaped character
  $string = str_replace('\\\\', '\\\\@', $string);
  // split array at ','-characters that are not preceded by a slash
  $array = preg_split('/(?<=[^\\\\]),|^,/', $string);
  $return_array = array();
  foreach ($array as $key => $value) {
    // split array at '='-characters that are not preceded by a slash
    $tokens = preg_split('/(?<=[^\\\\])=|^=/', $value);
    // not escaped '=' must occur exactly once, splitting the string into
    // exactly two strings
    if (sizeof($tokens) != 2) {
      return FALSE;
    }
    // undo first replacement for both tokens
    $parsed_key = str_replace('\\\\@', '\\\\', $tokens[0]);
    $parsed_value = str_replace('\\\\@', '\\\\', $tokens[1]);
    // unescape, i.e. remove slashes followed by a single character
    $parsed_key = preg_replace('/\\\\(.)/', '\\1', $parsed_key);
    $parsed_value = preg_replace('/\\\\(.)/', '\\1', $parsed_value);
    $return_array[$parsed_key] = $parsed_value;
  }
  return $return_array;
}

/**
 * Checks values of an array using user defined functions
 *
 * The check fails if the check for one value of the provided array fails. Note
 * that the check only succeeds if each keys of both provided arrays are the
 * same and appear in the same order. Moreover, the checks may have side-effects
 * on the input array. The input array is first copied before performing the
 * checks and copied back only if returning TRUE, that is the function is free
 * of side-effects when the check fails.
 *
 * @param array &$array key-value-pairs to be checked
 * @param array $checks mapping of keys to user defined functions
 * @return bool TRUE if the check succeeds, FALSE otherwise
 */
function drupaltodrupal_check_array(&$array, $checks) {
  if (!is_array($array)) {
    return FALSE;
  }
  if (array_keys($array) !== array_keys($checks)) {
    return FALSE;
  }
  $tmp_array = $array;
  foreach ($tmp_array as $key => &$value) {
    if (!call_user_func_array($checks[$key], array(&$value))) {
      return FALSE;
    }
  }
  $array = $tmp_array;
  return TRUE;
}
function drupaltodrupal_check_convert_int(&$x) {
  $y = intval($x);
  if ($x != strval($y)) {
    return FALSE;
  }
  else {
    $x = $y;
    return TRUE;
  }
}
