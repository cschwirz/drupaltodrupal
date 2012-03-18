<?php
/**
 * @file
 * Cryptographic functions.
 */

/**
 * Sign data
 */
function drupaltodrupal_sign($data, $private_key, $name) {
  if (openssl_sign($data, $signature, $private_key) === FALSE) {
    return FALSE;
  }
  else {
    $array = array(
      'data' => $data,
      'signature' => $signature,
      'signer' => $name,
    );
    return drupaltodrupal_implode($array);
  }
}

/**
 * Enrypt data
 */
function drupaltodrupal_encrypt($data, &$encrypted_data, $public_key) {
  if (openssl_seal($data, $sealed_data, $env_keys, array($public_key)) === FALSE) {
    return FALSE;
  }
  $env_key = $env_keys[0];
  $tmp = drupaltodrupal_implode(array('cipher' => $sealed_data, 'env_key' => $env_key));
  if ($tmp === FALSE) {
    return FALSE;
  }
  else
  {
    $encrypted_data = $tmp;
    return TRUE;
  }
}

