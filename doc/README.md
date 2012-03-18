Drupal-to-Drupal
================

For a general introduction to Drupal-to-Drupal refer to the README file in the top directory.
---

## D2D in action ##

### Managing instances ###
The _Instances_-tab gives an overview of all the instances available in the database. The first instance listed is the instance corresponding to the own instance. To manually add an instance to the database, the _Instances_-tab provides a form with an address and a description field. To insert an instance to the database, provide the address (starting with `http://` or `https://`) and ending with a single `/` and optionally add a description.

The top of the _Instances_-tab shows an overview of the instances in the database. To further customize (edit description, associate a public key, sending a friendship request etc.), click on the ID of an instance (first column).
The _Details_-sub tab allows the description of an instance to be changed. In the _Public Key_-sub tab, the public key associated with an instance can be set. To associate a public key with an instance, click on _receive public key_. The newly received public key will be listed as a possible candidate for the public key. After checking the public key, a candidate key can be chosen as public key for an instance. Furthermore there's the option to _Check friends' public keys_ which invokes a secure rpc on all the friends instances checking their public keys. The IDs of the candidate keys that are received from the friend instances are shown.

The _Friendship_ sub tab is used to manage the friendship with an instance. Friendship is certified via a friendship certificates: a certificate signed by the friend and a certificate by the own instance. Validity of these friendship certificates is shown in this sub tab. If both those friendship certificates are available (and valid), friendship between the two instances is valid and time until when the friendship is valid is shown. To invalidate (end) friendship, there is a button to remove all related certificates.
To send a friendship request, a form is provided at the bottom of the page. A message to be sent with the request can be provided. After sending the request (i.e. after clicking on _Request friendship_) the outgoing request is added to the queue of outgoing requests and will be automatically sent with the next cron job. If an outgoing request already exists in the queue, this request will simply be ignored. Furthermore note that for a request to be sent, a public key has to be associated with the instance. Finally, there's the possibility to send all outgoing requests manually, see _Settings_-tab for details.

The _Groups_-sub tab allows the managed instance to be put into groups. See the _Groups & Permissions_-tab for details.

The _Permissions_-sub-tab gives an overview of the functions that can be invoked by the instance. Note that these functions can only be called while friendship is valid. To change permissions of an instance, change the groups it belongs to and allow members of groups to call particular functions, see _Groups & Permissions_-tab for details.
Similarly, the _Remote Permissions_-sub tab gives an overview of the functions that can be invoked per remote on a friend instance. Again, note that the functions can only be invoked if friendship is valid. Furthermore listing permissions is only possible if allowed by the remote instance.

### Entries in the Database ###
The _Database_-tab gives a short overview of some entries stored in the database.

### Incoming requests ###
The _Incoming requests_-tabs lists incoming friendship requests that have been sent by other instances. An incoming request can only be accepted if the certificate for the friendship coming with this request and the signature of the request are valid. Note that validity can only be checked if a public key is associated with the requesting instance. After accepting a request, the friendship certificate coming with this request and a self signed certificate for the newly established friendship are added to the database. The later one will also be sent to the requesting instance. Note that the reply that accepts a request is not sent instantly but it is added to the queue of outgoing requests. Again, there is the possibility to send the reply manually (and immediately), see _Setting_-tab for details.

### Friends of friends ###
The _Friends of friends_-tab lists the instances that are friends of your friends and provides the possibility to directly add those friends of friends to the own database. After having added them to the database, an _edit / request friendship with referral_ link appears. This links allows to directly send a friendship request to the friend of friend instance providing a proof for the common friend. The instance the request has been sent to is now able to check the proof for the common friend and show the common friend as referral, if the check succeeded.

### Groups & Permissions ###
The _Groups & Permissions_-tab allows groups to be created and give permissions to members of the groups. In the _Groups_-sub tab, new groups can be created. Optionally, newly added instances can be added to a group. Furthermore, similar to instances, a description can be provided.
The _Permissions_-sub tab lists functions that can be called by friends on this instance and are defined by modules implementing `hook_drupaltodrupal_secure_rpc()`. The hook should return an array of methods that can be invoked. The key defines the name under which a remote function can be called, its value should be an array of key/value pairs, where the keys define `arguments`, `callback` and `description`. An example follows.

    /**
     * Implements hook_drupaltodrupal_secure_rpc().
     */
    function d2daddon_drupaltodrupal_secure_rpc() {
      $methods = array();
      $methods['d2daddon_remote_control'] = array(
        'arguments' => array('code' => 'is_string'),
        'callback' => 'd2daddon_srpc_remote_control',
        'description' => 'runs code on remote instance',
      );
      $methods['d2daddon_info'] = array(
        'arguments' => array(),
        'callback' => 'd2daddon_srpc_info',
        'description' => 'returns information about this instance',
      );
      return $methods;
    }

The value of the `callback`-key gives the name of a function being called internally when the specified function is called via remote. A description of how such a function should look like will follow.  The value belonging to the `arguments`-key has to be an array specifying the arguments being passed to the callback. The keys define the names of the arguments while the values specify functions being called for checking the corresponding argument. Therefore the function that checks the argument is called with the corresponding argument as reference argument and should return a boolean, in particular it should return `TRUE` if the argument is valid. Furthermore note that the argument gets its argument passed by reference and therefore cannot only do checks but also conversions.
An example of how a callback should look like follows.

    function d2daddon_srpc_remote_control($arguments, $rpc_info) {
      return eval($arguments['code']);
    }

The callback is given two arguments. The first argument is an array of arguments as defined in the hook and also as checked by the functions specified in the hook. The second argument is an associative array with keys `url`, `id`, `public_key`, `ip` corresponding to the address of the friend instance invoking the call, the internal id of the calling instance, the public key of the caller and the IP address the call came from. A proper return value of the callback function has to be of type string. For a function to return more advanced values than just strings, they have to be converted to a string, an example imploding an array to a string follows.

    function d2daddon_srpc_info($arguments, $rpc_info) {
      $friends = drupaltodrupal_get_friends();
      $n_friends = sizeof($friends);
      $software = $_SERVER['SERVER_SOFTWARE'];
      $phpversion = phpversion();
      $class = 'DatabaseTasks_' . Database::getConnection()->driver();
      $tasks = new $class();
      $dbname = '';
      $dbversion = '';
      $dbname = $tasks->name();
      $dbversion = Database::getConnection()->version();
      $return_array = array(
        'time' => date('l jS \of F Y h:i:s A'),
        'drupal version' => VERSION,
        'web server' => $software,
        'php version' => $phpversion,
        'database' => $dbname . $dbversion,
        'number of friends' => $n_friends,
      );
      foreach ($return_array as &$value) {
        $value = strval($value);
      }
      $imploded_return_array = drupaltodrupal_implode($return_array);
      if ($imploded_return_array === FALSE) {
        return '';
      }
      return $imploded_return_array;
    }

To call such a secure remote procedure on a friend instance, `drupaltodrupal_call_secure_rpc` has to be invoked. `drupaltodrupal_call_secure_rpc` takes four arguments: the first one specifies the friend to call that method on, the second one gives the name of the remote function (as defined in the corresponding hook on the remote instance). The third argument should be an associative array defining the arguments. Note that the keys have to exactly match and be in the same order as in the corresponding definition in the hook on the remote instance. The fourth and last argument is passed by reference. On error, this variable will contain the error string. Finally, on success `drupaltodrupal_call_secure_rpc` return the string being returned by the friend instance, `FALSE` otherwise. An example follows.

    function d2daddon_remote_control_form() {
      $form = array();
      $friends = drupaltodrupal_get_friends();
      if (empty($friends)) {
        drupal_set_message(t('No friends found in database'));
        return $form;
      }
      $options = array();
      $last_id = variable_get('d2daddon_remote_control_last_id');
      $proposed_id = null;
      foreach ($friends as $friend) {
        $options[$friend['id']] = $friend['url'];
        if (is_null($proposed_id) || $friend['id'] == $last_id) {
          $proposed_id = $friend['id'];
        }
      }
      $form['friend'] = array(
        '#type' => 'radios',
        '#title' => t('Instance to run code on'),
        '#default_value' => $proposed_id, 
        '#options' => $options,
      );
      $form['code'] = array(
        '#type' => 'textarea',
        '#title' => t('PHP Code to run'),
        '#description' => t('The provided code is evaluated using PHP\'s eval-function.'),
        '#rows' => 10,
        '#cols' => 60,
        '#default_value' => variable_get('d2daddon_remote_control_code', ''),
        '#required' => TRUE,
      );
      $form['submit'] = array(
        '#type' => 'submit', 
        '#value' => t('Run code'),
      );
      return $form;
    }
    function d2daddon_remote_control_form_submit($form, &$form_state) {
      variable_set('d2daddon_remote_control_code', $form_state['values']['code']);
      $friend_id = $form_state['values']['friend'];
      variable_set('d2daddon_remote_control_last_id', $friend_id);
      $friends = drupaltodrupal_get_friends();
      foreach ($friends as $friend) {
        if ($friend_id === $friend['id']) {
          $res = drupaltodrupal_call_secure_rpc($friend, 'd2daddon_remote_control', array('code' => $form_state['values']['code']), $error_string);
          if ($res === FALSE) {
            drupal_set_message(t('Error: @message', array('@message' => $error_string)));
          }
          else {
            drupal_set_message(t('Method returned \'@return\'.', array('@return' => $res)));
          }
          return;
        }
      }
      drupal_set_message('No friend selected.', 'warning');
    }

Note that with the installation a standard group is created with every new instance being added to this group by default. Permission to basic functions coming with D2D is given per default to members of this group.

### Settings ###
The _Settings_-tab gives various options to configure D2D. Description for the available options is provided with the corresponding form in the tab. Furthermore, at the bottom of the _Settings_-tab, there is button allowing all outgoing requests to be sent immediately.
