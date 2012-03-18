Drupal-to-Drupal
================

## Introduction ##
Drupal-to-Drupal (D2D) is a module to built-up a peer-to-peer network among Drupal instances using cryptography and XML-RPC.

An instance holds a public/private key pair allowing message to be encrypted and/or to be signed. The concept of friendship among Drupal instances is introduced. Therefore, instances exchange signed messages certifying their friendship. These friendship certificates have a limited validity but can be renewed automatically. Furthermore, friendship certificates allow friendship to be proved to other instances.
Using public key cryptography, an instance can implement secure XML-RPC methods that can be called by friend instances. Friends are organized in groups to allow privileged access to particular methods.

### What D2D provides ###
* built-up a peer-to-peer network
* add drupal instances as friends
* organize friends in groups
* easily implement remote functions and give privileged access to groups of friends

Since D2D is under development and mainly should be a proof of concept, there are many things / features, that are not provided, some features being possibly implemented in the future.
### What D2D does not provide (yet) ###
* the public/private key pair can not be changed / set, in particular in can be only be changed by the reinstallation of the module
* the address under which an instance is reachable cannot be changed, furthermore the suffix added to the address (`xmlrpc.php`) is fixed and cannot be changed
* instances cannot be removed (but of course friendship can be ended)
* the database is not locked before accessing, in particular concurrent access might corrupt data in the database
* D2D might be vulnerable to several remote attacks such as DoS-attacks
* ...

## Installation ##

### Requirements ###
* Drupal 7
* PHP 5.3 or higher
* PHP compiled with OpenSSL support
* MySQL database

### The installation ###
With the installation of the module a new public/private key pair will be created. Furthermore, the address under which this instance is reachable is set to `$GLOBALS['base_url'] . '/'`. Note that when installing D2D with drush the `-l` flag should be used, e.g. using the option `-l "http://www.example.com/drupal"`.
After installing D2D, a new tab called D2D appears in the admins menu. All functionality provided by D2D can be accessed using this tab.
Note that D2D requires Drupal's cron to be setup correctly and called periodically, e.g. every 15 minutes. See <http://drupal.org/cron> and <http://drupal.org/node/23714> for details on how to setup cron.