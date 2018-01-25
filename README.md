Nginx HTTP Set Lang Module
==========================

New Repo Location
=================

The new location for this repo is :

https://github.com/simplresty/ngx_http_set_lang

This old location was :

https://github.com/simpl/ngx_http_set_lang

This is one of a number of Nginx modules that have been moved from 'simpl' to 'simplresty'.
See [below](#see-also) for more information on how these organizations are now used.

To prevent any problems with updates, all repos that were previously under the 'simpl' user
will be forked and synced from the 'simplresty' repo. Sorry for any inconvenience this causes.

Description
===========

  An Nginx module that provides a variety of ways for setting a variable denoting the
  langauge that content should be returned in.


Methods for setting language variable
=====================================  

  - cookie
  - URL arguments
  - Accept-Language header
  - geoip
  - host
  - referer
  - POST variables (todo)


Usage
=====

  TODO


Installation
============

  ./configure --add-module=/path/to/ngx_devel_kit --add-module=/path/to/ngx_http_set_lang


Copyright
=========

  Marcus Clyne (c) 2010


License
=======

BSD


See Also
========

  * **[SimplResty](https://github.com/simplresty)** : A web-application framework integrating OpenResty, Couchbase, React
  and much more
  * **[Simpl](https://github.com/simpl)** : A shell platform that integrates repositories stored on Github, Bitbucket
  etc., to help facilitate shell-based tasks
