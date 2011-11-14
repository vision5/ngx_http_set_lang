Nginx HTTP Set Lang Module
--------------------------

Description
-----------

  An Nginx module that provides a variety of ways for setting a variable denoting the
  langauge that content should be returned in.


Methods for setting language variable
-------------------------------------  

  - cookie
  - URL arguments
  - Accept-Language header
  - geoip
  - host
  - referer
  - POST variables (todo)


Usage
-----

List the supported locales::

    lang_list en nl fr;

If you want to read from and write to cookies (``lang`` is the cookie name)::

    lang_cookie lang;

To make a (top-level) domain map to a certain locale::

    lang_host com en;

And to read the language from the user and put it in a variable::

    set_lang '$lang' accept_lang get post cookie geoip host referer default;


Working example::

    lang_list en pt_BR;
    lang_cookie lang;
    set_lang '$lang' cookie accept_lang default;

The example above uses accept-language to give you any supported language.
If no supported language is found than the ``$lang`` variable will be set to the first language in ``lang_list`` (i.e. ``en``).


Installation
------------

  ./configure --add-module=/path/to/ngx_devel_kit --add-module=/path/to/ngx_http_set_lang



Copyright
---------

  Marcus Clyne (c) 2010


License
-------

  BSD
