WHAT DOES IT DO?
----------------

This purpose of Omni EVE API is to incorporate and make use of the EVE Online
API for user registrations, role management, and site administration.

The module will pull the EVE API from an alliance executors API Key to grab
the current standings and to pull the corporations in the alliance. This
information is what is used as the standard to measure the EVE API data being
validated at registration.

When a user registers on the site, they are required to enter an API Key and
Verification code that is linked to their EVE Online Gaming character. The
site then processes checks to verify if the character meets certain criteria,
such as standings. If the character passes the criteria the user is allowed
to join and is added to the appropriate roles at registration.

TeamSpeak 3 and Jabber have been fully integrated. Users are able to register
on the site to gain roles and access in TeamSpeak 3 and Jabber without any
admin interaction. Roles are mirrored in TeamSpeak 3 and Jabber. Any role a
user has in Drupal will add the role to the user in TeamSpeak 3 and Jabber.

This module was inspired by the https://code.google.com/p/temars-eve-api/
Temars EVE API for SMF.

DEPENDENCIES
------------

 - Ctools (Only "Chaos tools" is required)
 https://drupal.org/project/ctools

 - Elysia Cron (Configure cron to run every minute via your hosts crontab)
 https://drupal.org/project/elysia_cron

 - Libraries
 https://drupal.org/project/libraries

 - TeamSpeak 3 (Optional)
 http://www.teamspeak.com/?page=downloads

 - Openfire (Jabber) (Optional)
 http://www.igniterealtime.org/downloads/index.jsp

 - cURL (libcurl For PHP) (Optional)
 http://www.php.net/manual/en/curl.installation.php

INSTALLATION
------------

 1. Download the TeamSpeak 3 PHP Framework.

    http://goo.gl/YJavKs

 2. Create the folder sites/all/libraries if it does not exist.

 3. Unpack the downloaded file, take the folder TeamSpeak3 located in the 
    libraries folder and upload it to your Drupal installation under
    sites/all/libraries.

    If done correctly you should see the file TeamSpeak3.php and various other
    files/folders when you navigate to sites/all/libraries/TeamSpeak3.

 4. Retrieve the lastest version of Omni EVE API from the git repository.

    http://drupalcode.org/sandbox/0mni/2043647.git

 5. Create the folder sites/all/modules/omni_eve_api if it does not exist.

 6. Upload all the files/folders from the git repository and upload it to your
    Drupal installation under sites/all/modules/omni_eve_api.

    If done correctly you should see the file omni_eve_api.module and various
    other files/folders when you navigate to sites/all/modules/omni_eve_api.

 7. Log in as an administrator on your Drupal site and go to the Modules page
    at admin/modules. Enable Omni EVE API listed under Omni EVE API.

 8. Still logged in as an administrator, go to EVE API page at
    admin/settings/omni_eve_api. Registrations will be disabled until a valid
    Alliance API Key is entered. Create or retrieve your Alliance API Key from
    http://goo.gl/LDks44 and enter the "Key ID" and "Verification Code" in the
    appropriate fields. Be sure to tick the check box "Enable Omni EVE API" and
    click on "Update". It may take up to a minute for the cron task to run and
    pull the data from the EVE API. Once data has been successfully retrieved
    the module will be enabled and user registrations will be turned back on.

CONFIGURE TEAMSPEAK 3
---------------------

 1. Log in as an administrator on your Drupal site and go to the EVE API
    TeamSpeak 3 settings page at admin/settings/omni_eve_api/teamspeak.

 2. Enter all the information as requested in the fields. Be sure to tick the
    check box "Enable TeamSpeak 3 Connection" and click on "Submit".

 3. You will be notified if the connection to the TeamSpeak 3 server is
    successful.

 4. If the TeamSpeak 3 connection is successful, you will have the option to
    select a Bypass Group. Users in the Bypass Group will not be pestered or
    kicked if not registered or if the name is not correct on the server.

CONFIGURE OPENFIRE (JABBER)
---------------------------

 1. Log in as an administrator on your Drupal site and go to the EVE API
    Jabber settings page at admin/settings/omni_eve_api/jabber.

 2. Enter all the information as requested in the fields. Be sure to tick the
    check box "Enable Jabber Connection" and click on "Submit".

 3. You will be notified if the connection to the Jabber server is successful.
