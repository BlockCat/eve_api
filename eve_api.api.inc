<?php
/**
 * @file
 * Functions related to retrieving and manipulating data from the EVE API.
 */

/**
 * Function to retrieve the XML.
 *
 * @param string $type
 *   Indicate what URL to use to retrieve the XML.
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return mixed
 *   Returns a valid XML object or ERROR information.
 */
function eve_api_get_xml($type, $query = array()) {
  $url = 'https://api.eveonline.com';

  switch ($type) {
    case 'ContactList':
      $url .= "/corp/ContactList.xml.aspx";
      break;

    case 'AllianceList':
      $url .= "/eve/AllianceList.xml.aspx";
      break;

    case 'CorporationSheet':
      $url .= "/corp/CorporationSheet.xml.aspx";
      break;

    case 'CharacterSheet':
      $url .= "/char/CharacterSheet.xml.aspx";
      break;

    case 'FacWarStats':
      $url .= "/char/FacWarStats.xml.aspx";
      break;

    case 'CharacterID':
      $url .= "/eve/CharacterID.xml.aspx";
      break;

    case 'CharacterName':
      $url .= "/eve/CharacterName.xml.aspx";
      break;

    case 'CharacterInfo':
      $url .= "/eve/CharacterInfo.xml.aspx";
      break;

    case 'callList':
      $url .= "/api/callList.xml.aspx";
      break;

    case 'SkillTree':
      $url .= "/eve/SkillTree.xml.aspx";
      break;

    default:
    case 'APIKeyInfo':
      $url .= "/account/APIKeyInfo.xml.aspx";
      break;
  }

  $cache_name = 'xml' . $type . str_replace('=', '', str_replace('&', '', drupal_http_build_query($query)));
  $html = '';

  if ($cached = cache_get($cache_name, 'cache')) {
    $html = $cached->data;
    $error = 200;
  }

  if (empty($html) || $type == 'SkillTree') {
    if (variable_get('eve_api_cron_error_count', 0) >= variable_get('eve_api_cron_error_count_limit', 5)) {
      variable_set('eve_api_cron_error_time', time() + variable_get('eve_api_cron_error_time_limit', 10 * 60));
      variable_set('eve_api_cron_error_count', 0);
      watchdog('eve_api', 'EVE API has failed 5 times to pull cron data, API cron resumes at ' . date('Y-m-d H:i:s', time() + variable_get('eve_api_cron_error_time_limit', 10 * 60)), NULL, WATCHDOG_WARNING);
      return 2;
    }
    elseif (variable_get('eve_api_cron_error_time', 0) >= time()) {
      return 2;
    }

    $has_curl = function_exists('curl_init');
    $open_basedir = ini_get('open_basedir');

    if (!$has_curl || $open_basedir) {
      $options = array(
        'method' => 'POST',
        'data' => drupal_http_build_query($query),
        'timeout' => 45,
        'headers' => array('Content-Type' => 'application/x-www-form-urlencoded'),
      );

      $html = drupal_http_request($url, $options);
    }
    else {
      $html = eve_api_curl_http_request($url, $query);
    }

    if (!empty($html->error)) {
      watchdog('eve_api', 'EVE API returned an error with the HTTP request. (' . $type . '?' . drupal_http_build_query($query) . ') Error: ' . $html->error . ' Code: ' . $html->code, NULL, WATCHDOG_WARNING);
    }

    $error = $html->code;
  }

  // Fix for HTTP_REQUEST_TIMEOUT returning as -1
  if ($error == -1) {
    return 3;
  }

  if ($error < 400) {
    if (FALSE !== strpos($html->data, '<!DOCTYPE html')) {
      watchdog('eve_api', 'EVE API returned an error with the HTTP request. (' . $type . '?' . drupal_http_build_query($query) . ') Error: ' . $html->error . ' Code: ' . $html->code, NULL, WATCHDOG_WARNING);
      $error = 4;
    }
    else {
      try {
        $xml = @new SimpleXMLElement($html->data);
      }
      catch (Exception $e) {
        watchdog('eve_api', 'EVE API returned an error with the HTTP request. (' . $type . '?' . drupal_http_build_query($query) . ') Error: ' . $e->getMessage(), NULL, WATCHDOG_WARNING);
        $error = 5;
        $xml = NULL;
      }
    }
  }

  if (isset($xml->cachedUntil)) {
    cache_set($cache_name, $html, 'cache', strtotime($xml->cachedUntil));
    return $xml;
  }

  if (isset($xml->errorCode)) {
    watchdog('eve_api', 'EVE API returned an error with the API data. (' . $type . '?' . drupal_http_build_query($query) . ') Error: ' . $xml->errorCode, NULL, WATCHDOG_WARNING);
    $error = $xml->errorCode;
  }

  variable_set('eve_api_cron_error_count', 1 + variable_get('eve_api_cron_error_count', 0));

  return $error;
}

/**
 * Performs an HTTP request.
 *
 * This is a flexible and powerful HTTP client implementation. Correctly
 * handles GET, POST, PUT or any other HTTP requests. Handles redirects.
 * Based on "cURL HTTP Request v1.5" by Vincenzo and minorOffense
 * Based on "Yapeal (http://code.google.com/p/yapeal/)" by Kevin Burkholder
 * Modified for use with "Omni EVE API"
 *
 *
 * @param string $url
 *   A string containing a fully qualified URI.
 * @param array $query
 *   (optional) An array composed of the url variable elements.
 * @param string $method
 *   (optional) POST, GET, PULL.
 *
 * @return object
 *   An object that can have one or more of the following components:
 *   - request: A string containing the request body that was sent.
 *   - code: An integer containing the response status code, or the error code
 *     if an error occurred.
 *   - protocol: The response protocol (e.g. HTTP/1.1 or HTTP/1.0).
 *   - status_message: The status message from the response, if a response was
 *     received.
 *   - redirect_code: If redirected, an integer containing the initial response
 *     status code.
 *   - redirect_url: If redirected, a string containing the URL of the redirect
 *     target.
 *   - error: If an error occurred, the error message. Otherwise not set.
 *   - errno: If an error occurred, a cURL error number greater than 0.
 *     Otherwise set to 0.
 *   - headers: An array containing the response headers as name/value pairs.
 *     HTTP header names are case-insensitive (RFC 2616, section 4.2), so for
 *     easy access the array keys are returned in lower case.
 *   - data: A string containing the response body that was received.
 */
function eve_api_curl_http_request($url, array $query = array(), $method = 'POST') {
  $result = new stdClass();

  // Parse the URL and make sure we can handle the schema.
  $uri = @parse_url($url);

  if ($uri == FALSE) {
    $result->error = 'unable to parse URL';
    $result->code = -1001;
    return $result;
  }

  if (!isset($uri['scheme'])) {
    $result->error = 'missing schema';
    $result->code = -1002;
    return $result;
  }

  timer_start(__FUNCTION__);

  // Merge the default options.
  $options = array(
    'data' => $query,
    'headers' => array(),
    'method' => $method,
    'timeout' => 45,
    'http_proxy' => variable_get('http_proxy'),
    'https_proxy' => variable_get('https_proxy'),
  );

  // Select the right proxy for the right protocol.
  $proxy = ('https' == $uri['scheme']) ? $options['https_proxy'] : $options['http_proxy'];

  // Nullify the proxy if the host to send the request to is part of the proxy's
  // exceptions.
  if ((!empty($proxy['exceptions'])) && (array_key_exists($uri['host'], $proxy['exceptions']))) {
    $proxy = NULL;
  }

  $curl = curl_version();
  $user_agent = 'Drupal (+http://drupal.org/)';
  $user_agent .= ' Omni EVE API/' . eve_api_version();
  $user_agent .= ' (' . PHP_OS . ' ' . php_uname('m') . ')';
  $user_agent .= ' libcurl/' . $curl['version'];
  $user_agent = trim($user_agent);

  $curl_opt = array(
    CURLOPT_HEADER => TRUE,
    CURLINFO_HEADER_OUT => TRUE,
    CURLOPT_TIMEOUT => $options['timeout'],
    CURLOPT_FOLLOWLOCATION => TRUE,
    CURLOPT_RETURNTRANSFER => TRUE,
    CURLOPT_MAXREDIRS => 5,
    CURLOPT_NOBODY => FALSE,
    CURLOPT_FORBID_REUSE => FALSE,
    CURLOPT_LOW_SPEED_LIMIT => 10,
    CURLOPT_LOW_SPEED_TIME => ceil($options['timeout'] / 4),
    CURLOPT_MAXCONNECTS => 5,
    CURLOPT_CONNECTTIMEOUT => ceil($options['timeout'] / 2),
    CURLOPT_REFERER => 'http://danlee.ca/git/',
    CURLOPT_UNRESTRICTED_AUTH => TRUE,
    CURLOPT_ENCODING => 'gzip',
    CURLOPT_BINARYTRANSFER => FALSE,
    CURLOPT_USERAGENT => $user_agent,
    CURLOPT_SSL_VERIFYPEER => TRUE,
    CURLOPT_SSL_VERIFYHOST => 2,
    CURLOPT_CAINFO => drupal_get_path('module', 'eve_api') . '/eveonline.crt',
    CURLOPT_COOKIEJAR => drupal_get_path('module', 'eve_api') . '/cookies.txt',
  );

  $ssl_version = $curl['ssl_version'];
  $has_nss = (strpos($ssl_version, "NSS") > -1);

  if ($has_nss) {
    $curl_opt += array(CURLOPT_SSL_CIPHER_LIST => 'rsa_aes_128_sha,rsa_aes_256_sha,rsa_3des_sha,rsa_rc4_128_sha,rsa_rc4_128_md5');
  }
  else {
    $curl_opt += array(CURLOPT_SSL_CIPHER_LIST => 'AES128-SHA AES256-SHA DES-CBC3-SHA RC4-SHA RC4-MD5');
  }

  if (!empty($proxy)) {
    $proxy_options = array(
      CURLOPT_PROXY => $proxy['server'],
      CURLOPT_PROXYPORT => $proxy['port'],
    );
    // For the time being let's just support HTTP proxies with basic
    // authentication.
    if (isset($proxy['username']) && isset($proxy['password'])) {
      $proxy_options += array(
        CURLOPT_PROXYUSERPWD => implode(':', array(
          $proxy['username'],
          $proxy['password'],
        )),
        CURLOPT_PROXYTYPE => CURLPROXY_HTTP,
        CURLOPT_PROXYAUTH => CURLAUTH_BASIC,
      );
    }
    $curl_opt += $proxy_options;
  }

  $default_ports = array(
    'http' => 80,
    'feed' => 80,
    'https' => 443,
  );

  if (array_key_exists($uri['scheme'], $default_ports)) {
    if (!isset($uri['port'])) {
      $uri['port'] = $default_ports[$uri['scheme']];
    }
    // RFC 2616: "non-standard ports MUST, default ports MAY be included".
    // We don't add the standard port to prevent from breaking rewrite rules
    // checking the host that do not take into account the port number.
    $options['headers']['Host'] = $uri['host'] . ($uri['port'] != 80 ? ':' . $uri['port'] : '');
  }
  else {
    $result->error = 'invalid schema ' . $uri['scheme'];
    $result->code = -1003;
    return $result;
  }

  // Merge the default headers.
  $options['headers'] += array(
    'User-Agent' => 'Drupal (+http://drupal.org/)',
  );

  // Only add Content-Length if we actually have any content or if it is a POST
  // or PUT request. Some non-standard servers get confused by Content-Length in
  // at least HEAD/GET requests, and Squid always requires Content-Length in
  // POST/PUT requests.
  $content_length = strlen(drupal_http_build_query($options['data']));
  if ($content_length > 0 || $options['method'] == 'POST' || $options['method'] == 'PUT') {
    $options['headers']['Content-Length'] = $content_length;
  }

  // If the server URL has a user then attempt to use basic authentication.
  if (isset($uri['user'])) {
    $options['headers']['Authorization'] = 'Basic ' . base64_encode($uri['user'] . (isset($uri['pass']) ? ':' . $uri['pass'] : ''));
  }

  // Set all the headers.
  $curl_opt[CURLOPT_HTTPHEADER] = array();
  foreach ($options['headers'] as $name => $value) {
    $curl_opt[CURLOPT_HTTPHEADER][] = $name . ": " . trim($value);
  }

  // Set the request method.
  switch ($options['method']) {
    case 'GET':
      $curl_opt[CURLOPT_HTTPGET] = TRUE;
      break;

    case 'POST':
      $curl_opt[CURLOPT_POST] = TRUE;
      if (!empty($options['data'])) {
        $curl_opt[CURLOPT_POSTFIELDS] = drupal_http_build_query($options['data']);
        $options['headers'] += array(
          'Content-Type' => 'application/x-www-form-urlencoded',
        );
      }
      break;

    case 'PUT':
      $curl_opt[CURLOPT_PUT] = TRUE;
      break;

    default:
      $result->error = 'invalid method ' . $options['method'];
      $result->code = -1004;
      return $result;
  }

  // Make the request.
  $ch = curl_init($url);
  curl_setopt_array($ch, $curl_opt);

  // Full response stored. To be parsed later on and split in protocol, code,
  // status message, and response headers.
  $result->data = trim(curl_exec($ch));
  $result->error = curl_error($ch);
  $result->errno = curl_errno($ch);

  // If there's been an error, do not continue.
  if ($result->error) {
    // Request timed out.
    if (CURLE_OPERATION_TIMEOUTED == $result->errno) {
      $result->code = HTTP_REQUEST_TIMEOUT;
      $result->error = 'request timed out';
      return $result;
    }
    $result->code = $result->errno;
    return $result;
  }

  // The last effective URL should correspond to the Redirect URL.
  $result->redirect_url = curl_getinfo($ch, CURLINFO_EFFECTIVE_URL);

  // Save the request sent into the result object.
  $result->request = curl_getinfo($ch, CURLINFO_HEADER_OUT);

  // Parse response headers from the response body.
  // Be tolerant of malformed HTTP responses that separate header and body with
  // \n\n or \r\r instead of \r\n\r\n.
  list($response, $result->data) = preg_split("/\r\n\r\n|\n\n|\r\r/", $result->data, 2);

  // Sometimes when making an HTTP request via proxy using cURL, you end up with
  // a multiple set of headers:
  // from the web server being the actual target, from the proxy itself, etc.
  // The following 'if' statement is to check for such a situation and make sure
  // we get a proper split between
  // actual response body and actual response headers both coming from the web
  // server.
  while ('HTTP/' == substr($result->data, 0, 5)) {
    list($response, $result->data) = preg_split("/\r\n\r\n|\n\n|\r\r/", $result->data, 2);
  }

  $response = preg_split("/\r\n|\n|\r/", $response);

  // Parse the response status line.
  list($protocol, $code, $status_message) = explode(' ', trim(array_shift($response)), 3);
  $result->protocol = $protocol;
  $result->status_message = $status_message;

  $result->headers = array();

  // Parse the response headers.
  while ($line = trim(array_shift($response))) {
    list($name, $value) = explode(':', $line, 2);
    $name = strtolower($name);
    if (isset($result->headers[$name]) && $name == 'set-cookie') {
      // RFC 2109: the Set-Cookie response header comprises the token Set-
      // Cookie:, followed by a comma-separated list of one or more cookies.
      $result->headers[$name] .= ',' . trim($value);
    }
    else {
      $result->headers[$name] = trim($value);
    }
  }

  $responses = array(
    100 => 'Continue',
    101 => 'Switching Protocols',
    200 => 'OK',
    201 => 'Created',
    202 => 'Accepted',
    203 => 'Non-Authoritative Information',
    204 => 'No Content',
    205 => 'Reset Content',
    206 => 'Partial Content',
    300 => 'Multiple Choices',
    301 => 'Moved Permanently',
    302 => 'Found',
    303 => 'See Other',
    304 => 'Not Modified',
    305 => 'Use Proxy',
    307 => 'Temporary Redirect',
    400 => 'Bad Request',
    401 => 'Unauthorized',
    402 => 'Payment Required',
    403 => 'Forbidden',
    404 => 'Not Found',
    405 => 'Method Not Allowed',
    406 => 'Not Acceptable',
    407 => 'Proxy Authentication Required',
    408 => 'Request Time-out',
    409 => 'Conflict',
    410 => 'Gone',
    411 => 'Length Required',
    412 => 'Precondition Failed',
    413 => 'Request Entity Too Large',
    414 => 'Request-URI Too Large',
    415 => 'Unsupported Media Type',
    416 => 'Requested range not satisfiable',
    417 => 'Expectation Failed',
    500 => 'Internal Server Error',
    501 => 'Not Implemented',
    502 => 'Bad Gateway',
    503 => 'Service Unavailable',
    504 => 'Gateway Time-out',
    505 => 'HTTP Version not supported',
  );
  // RFC 2616 states that all unknown HTTP codes must be treated the same as the
  // base code in their class.
  if (!isset($responses[$code])) {
    $code = floor($code / 100) * 100;
  }
  $result->code = $code;

  switch ($code) {
    case 200:
      // OK.
    case 304:
      // Not modified.
      break;

    case 301:
      // Moved permanently.
    case 302:
      // Moved temporarily.
    case 307:
      // Moved temporarily.
      $location = $result->headers['location'];
      $options['timeout'] -= timer_read(__FUNCTION__) / 1000;
      if ($options['timeout'] <= 0) {
        $result->code = HTTP_REQUEST_TIMEOUT;
        $result->error = 'request timed out';
      }
      elseif ($options['max_redirects']) {
        // Redirect to the new location.
        $options['max_redirects']--;
        $result = curl_http_request($location, $options);
        $result->redirect_code = $code;
      }
      if (!isset($result->redirect_url)) {
        $result->redirect_url = $location;
      }
      break;

    default:
      $result->error = $status_message;
  }

  curl_close($ch);
  return $result;
}

/**
 * Function to retrieve individual characters from the database.
 *
 * @param string $character_name
 *   Unique character name used to retrieve information about the character
 *
 * @return array
 *   A complex array of a single character information.
 */
function eve_api_get_character_info($character_name) {
  db_query('SET group_concat_max_len = 1048576');
  $result = db_query('SELECT c.uid, c.deleted, c.errorID, c.apiID, c.characterName, c.characterID, c.corporationName, c.corporationID, c.corporationTicker, c.allianceID, c.allianceName, c.allianceTicker,
    cs.DoB, cs.race, cs.bloodLine, cs.ancestry, cs.gender, cs.cloneName, cs.cloneSkillPoints, cs.balance, cs.intelligence, cs.memory, cs.charisma, cs.perception, cs.willpower,
    ci.skillPoints, ci.shipName, ci.shipTypeID, ci.shipTypeName, ci.corporationDate, ci.allianceDate, ci.lastKnownLocation, ci.securityStatus,
    GROUP_CONCAT( DISTINCT CONCAT(csae.bonusName, \'~\', csae.augmentatorValue) SEPARATOR \'|\') AS bonusName,
    GROUP_CONCAT( DISTINCT CONCAT(\'recordID!\', cich.recordID, \'~\', \'corporationID!\', cich.corporationID, \'~\', \'startDate!\', cich.startDate) SEPARATOR \'|\') AS history
    FROM {eve_api_characters} c
    INNER JOIN {eve_api_character_sheet} cs ON cs.characterID = c.characterID
    INNER JOIN {eve_api_character_info} ci ON ci.characterID = c.characterID
    LEFT JOIN {eve_api_character_sheet_attribute_enhancers} csae on csae.characterID = c.characterID
    LEFT JOIN {eve_api_character_info_corporation_history} cich on cich.characterID = c.characterID
    WHERE c.characterName = :characterName', array(
      ':characterName' => (string) $character_name,
    ));

  $api_data = array();

  if ($result->rowCount()) {
    foreach ($result->fetchAssoc() as $name => $data) {
      if ($name == 'bonusName') {
        $bonus_rows = explode('|', $data);

        foreach ($bonus_rows as $bonus_row) {
          $bonus_data = explode('~', $bonus_row);

          if (empty($bonus_data[0])) {
            continue;
          }

          $api_data += array($bonus_data[0] => strip_tags(decode_entities($bonus_data[1])));
        }

        continue;
      }

      if ($name == 'history') {
        $history_results = explode('|', $data);

        foreach ($history_results as $history_result) {
          $history_rows = explode('~', $history_result);
          $history_corp = array();

          foreach ($history_rows as $history_row) {
            $history_data = explode('!', $history_row);

            if (empty($history_data[0])) {
              continue;
            }

            $history_corp += array($history_data[0] => strip_tags(decode_entities($history_data[1])));
          }

          $api_data['history'][] = $history_corp;
        }
        continue;
      }

      $api_data += array($name => strip_tags(decode_entities($data)));
    }
  }
  else {
    $api_data = FALSE;
  }

  return $api_data;
}

/**
 * Function to retrieve individual characters skills from the database.
 *
 * @param int $character_id
 *   Unique character id used to retrieve information about the character
 *
 * @return array
 *   A complex array of a single character skills.
 */
function eve_api_get_character_skills($character_id) {
  $result = db_query('SELECT css.level, css.skillpoints, st.typeName, sg.groupName, sg.groupID, st.typeID
    FROM {eve_api_character_sheet_skills} css
    INNER JOIN {eve_api_sheet_types} st ON st.typeID = css.typeID
    INNER JOIN {eve_api_sheet_groups} sg ON sg.groupID = st.groupID
    WHERE css.characterID = :characterID ORDER BY sg.groupName, st.typeName ASC', array(
      ':characterID' => (int) $character_id,
    ));

  $api_data = array();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $api_data[(string) $row->groupName][] = array(
        'level' => check_plain((int) $row->level),
        'groupID' => check_plain((int) $row->groupID),
        'typeID' => check_plain((int) $row->typeID),
        'typeName' => check_plain((string) $row->typeName),
        'skillpoints' => check_plain((int) $row->skillpoints),
      );
    }
  }
  else {
    $api_data = FALSE;
  }

  return $api_data;
}

/**
 * Function to retrieve individual characters API info.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A complex array of multiple character information.
 */
function eve_api_get_character_api($query) {
  $xml = eve_api_get_xml('APIKeyInfo', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Character API Data. (' . drupal_http_build_query($query) . ') Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);
    return array('error' => (int) $xml);
  }

  $api_data = array();
  $api_info = $xml->result->key->attributes();
  $api_data['accessMask'] = (int) $api_info->accessMask;
  $api_data['type'] = (string) $api_info->type;
  $api_data['expires'] = ((string) $api_info->expires) ? TRUE : FALSE;

  if (isset($xml->errorCode)) {
    $api_data['errorCode'] = (int) $xml->errorCode;
  }

  foreach ($xml->result->key->rowset->children() as $rows) {
    $row = $rows->attributes();

    $query = array('corporationID' => (int) $row->corporationID);
    $corp_info = eve_api_get_corporation_info($query);

    if (isset($corp_info['error'])) {
      return array('error' => $corp_info);
    }
    else {
      $api_data['characters'][(int) $row->characterID] = array_merge(array(
        'characterName' => (string) $row->characterName,
        'characterID' => (int) $row->characterID,
        'corporationName' => (string) $row->corporationName,
        'corporationID' => (int) $row->corporationID,
      ), $corp_info);
    }
  }

  return $api_data;
}

/**
 * Function to retrieve a single characters character info.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A complex array of character information.
 */
function eve_api_get_character_info_api($query) {
  $xml = eve_api_get_xml('CharacterInfo', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Character Sheet API Data. (' . drupal_http_build_query($query) . ') Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);
    return array('error' => (int) $xml);
  }

  $api_data = array();

  if (isset($xml->errorCode)) {
    $api_data['errorCode'] = (int) $xml->errorCode;
  }

  foreach ($xml->result->children() as $results) {
    switch ((string) $results->getName()) {
      case 'rowset':
        foreach ($results->children() as $rows) {
          $row = $rows->attributes();

          $api_data['employmentHistory'][(int) $row->recordID] = array(
            'recordID' => (int) $row->recordID,
            'corporationID' => (int) $row->corporationID,
            'startDate' => (string) $row->startDate,
          );
        }
        break;

      default:
        $api_data['info'][(string) $results->getName()] = (string) $results;
        break;
    }
  }

  return $api_data;
}

/**
 * Function to retrieve a single characters character sheet.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A complex array of character sheet information.
 */
function eve_api_get_character_sheet_api($query) {
  $xml = eve_api_get_xml('CharacterSheet', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Character Sheet API Data. (' . drupal_http_build_query($query) . ') Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);
    return array('error' => (int) $xml);
  }

  $api_data = array();

  if (isset($xml->errorCode)) {
    $api_data['errorCode'] = (int) $xml->errorCode;
  }

  foreach ($xml->result->children() as $results) {
    switch ((string) $results->getName()) {
      case 'rowset':
        $rowset = $results->attributes();

        foreach ($results->children() as $rows) {
          $row = $rows->attributes();

          switch ((string) $rowset->name) {
            case 'skills':
              $api_data['skills'][] = array(
                'typeID' => (int) $row->typeID,
                'skillpoints' => (int) $row->skillpoints,
                'level' => (int) $row->level,
                'published' => (int) $row->published,
              );
              break;

            case 'certificates':
              $api_data['certificates'][]['certificateID'] = (int) $row->certificateID;
              break;

            case 'corporationTitles':
              $api_data['corporationTitles'][] = array(
                'titleID' => (int) $row->titleID,
                'titleName' => (string) $row->titleName,
              );
              break;

            default:
              $api_data['roles'][(string) $rowset->name][] = array(
                'roleID' => (int) $row->roleID,
                'roleName' => (string) $row->roleName,
              );
              break;
          }
        }
        break;

      case 'attributeEnhancers':
        foreach ($results->children() as $rows) {
          foreach ($rows->children() as $row) {
            $api_data['attributeEnhancers'][(string) $rows->getName()][(string) $row->getName()] = (string) $row;
          }
        }
        break;

      case 'attributes':
        foreach ($results->children() as $rows) {
          $api_data['info'][(string) $rows->getName()] = (int) $rows;
        }
        break;

      default:
        $api_data['info'][(string) $results->getName()] = (string) $results;
        break;
    }
  }

  return $api_data;
}

/**
 * Function to retrieve skill definitions.
 *
 * @return array
 *   A complex array of skill definitions.
 */
function eve_api_get_skill_tree() {
  $xml = eve_api_get_xml('SkillTree');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Skill Tree API Data. Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);
    return array('error' => (int) $xml);
  }

  $api_data = array();

  if (isset($xml->errorCode)) {
    $api_data['errorCode'] = (int) $xml->errorCode;
  }

  foreach ($xml->result->rowset->children() as $rowsets) {
    $rowset = $rowsets->attributes();

    $api_data['group'][(int) $rowset->groupID] = array(
      'groupID' => (int) $rowset->groupID,
      'groupName' => (string) $rowset->groupName,
    );

    foreach ($rowsets->rowset->children() as $rows) {
      $row = $rows->attributes();

      $api_data['type'][(int) $row->typeID] = array(
        'typeID' => (int) $row->typeID,
        'typeName' => (string) $row->typeName,
        'groupID' => (int) $row->groupID,
      );
    }
  }

  return $api_data;
}

/**
 * Function to retrieve the alliance tags.
 *
 * @param int $alliance_id
 *   The Unique ID for an alliance.
 *
 * @return string
 *   The Alliance ticker for the $allianceID.
 */
function eve_api_get_alliance_ticker($alliance_id) {
  $result = db_query('SELECT allianceTicker FROM {eve_api_alliance_info} WHERE allianceID = :id', array(
    ':id' => (int) $alliance_id,
  ));
  $row = $result->fetchAssoc();

  return $row['allianceTicker'];
}

/**
 * Function to retrieve corporation API Info.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A complex array of corporate information.
 */
function eve_api_get_corporation_info($query) {
  $xml = eve_api_get_xml('CorporationSheet', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Corporation API Data. (' . drupal_http_build_query($query) . ') Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  $api_data = array(
    'corporationID' => (int) $xml->result->corporationID,
    'corporationName' => (string) $xml->result->corporationName,
    'corporationTicker' => (string) $xml->result->ticker,
    'allianceID' => (int) $xml->result->allianceID,
    'allianceName' => (string) $xml->result->allianceName,
    'allianceTicker' => (string) eve_api_get_alliance_ticker((int) $xml->result->allianceID),
    'ceoName' => (string) $xml->result->ceoName,
    'ceoID' => (int) $xml->result->ceoID,
  );

  if (isset($xml->errorCode)) {
    $api_data = $api_data + array('errorCode' => (string) $xml->errorCode);
  }

  return $api_data;
}

/**
 * Function to retrieve Character/Alliance/Corporation ID's from a name.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A simple array containing unique name and ID information.
 */
function eve_api_get_unique_id($query) {
  $xml = eve_api_get_xml('CharacterID', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Character ID API Data. (' . drupal_http_build_query($query) . ') Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  $api_data = array();

  foreach ($xml->result->rowset->children() as $rows) {
    $row = $rows->attributes();
    $name = strtoupper((string) $row->name);

    $api_data[$name] = array(
      'name' => (string) $row->name,
      'characterID' => (int) $row->characterID,
    );
  }

  if (isset($xml->errorCode)) {
    $api_data['errorCode'] = (int) $xml->errorCode;
  }

  return $api_data;
}

/**
 * Function to retrieve character names from character ids.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A simple array containing unique name and ID information.
 */
function eve_api_get_character_name($query) {
  $xml = eve_api_get_xml('CharacterName', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Character Name API Data. (' . drupal_http_build_query($query) . ') Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  $api_data = array();

  foreach ($xml->result->rowset->children() as $rows) {
    $row = $rows->attributes();

    $api_data[(int) $row->characterID] = (string) $row->name;
  }

  if (isset($xml->errorCode)) {
    $api_data['errorCode'] = (int) $xml->errorCode;
  }

  return $api_data;
}

/**
 * Function to compare the API against the Alliance Blue List.
 *
 * @param array $characters
 *   A complex array full of character information.
 *
 * @return bool
 *   Indicates a character is blue to the alliance.
 */
function eve_api_verify_blue($characters) {
  if (isset($characters['characterID'])) {
    if (($characters['allianceID'] == variable_get('eve_api_allianceID', 0) && variable_get('eve_api_allianceID', 0) != 0) || $characters['corporationID'] == variable_get('eve_api_corporationID', 0)) {
      return TRUE;
    }

    $result = db_query('SELECT corporationID FROM {eve_api_alliance_corporations} WHERE corporationID = :corporationID', array(
      ':corporationID' => (int) $characters['corporationID'],
    ));

    if ($result->rowCount()) {
      return TRUE;
    }

    $result = db_query('SELECT contactID FROM {eve_api_blue_standings} WHERE standing >= :standing AND contactID IN (:contactIDs)', array(
      ':standing' => (float) variable_get('eve_api_required_standing', 0.1),
      ':contactIDs' => array(
        (int) $characters['characterID'],
        (int) $characters['corporationID'],
        (int) $characters['allianceID'],
      ),
    ));

    if ($result->rowCount()) {
      return TRUE;
    }
  }
  elseif (isset($characters['characters'])) {
    foreach ($characters['characters'] as $character) {
      if (($character['allianceID'] == variable_get('eve_api_allianceID', 0) && variable_get('eve_api_allianceID', 0) != 0) || $character['corporationID'] == variable_get('eve_api_corporationID', 0)) {
        return TRUE;
      }

      $result = db_query('SELECT corporationID FROM {eve_api_alliance_corporations} WHERE corporationID = :corporationID', array(
        ':corporationID' => (int) $character['corporationID'],
      ));

      if ($result->rowCount()) {
        return TRUE;
      }

      $result = db_query('SELECT contactID FROM {eve_api_blue_standings} WHERE standing >= :standing AND contactID IN (:contactIDs)', array(
        ':standing' => (float) variable_get('eve_api_required_standing', 0.1),
        ':contactIDs' => array(
          (int) $character['characterID'],
          (int) $character['corporationID'],
          (int) $character['allianceID'],
        ),
      ));

      if ($result->rowCount()) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

/**
 * Function to generate an array of Blue Characters to the Alliance.
 *
 * @param array $characters
 *   A complex array full of character information.
 *
 * @return array
 *   A simple array of characters blue to the alliance.
 */
function eve_api_list_valid_characters($characters) {
  $chars = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $result = db_query('SELECT contactID FROM {eve_api_blue_standings} WHERE standing > 0 AND contactID IN (:contactIDs)', array(
        ':contactIDs' => array(
          (int) $character['characterID'],
          (int) $character['corporationID'],
          (int) $character['allianceID'],
        ),
      ));

      if ($result->rowCount()) {
        $chars[] = $character['characterName'];
        continue;
      }

      $result = db_query('SELECT corporationID FROM {eve_api_alliance_corporations} WHERE corporationID = :corporationID', array(
        ':corporationID' => (int) $character['corporationID'],
      ));

      if ($result->rowCount()) {
        $chars[] = $character['characterName'];
      }
    }
  }

  return $chars;
}

/**
 * Function to check if the Characters already exist in the DB.
 *
 * @param array $characters
 *   A complex array full of character information.
 *
 * @return mixed
 *   A simple array of characters found or a bool FALSE.
 */
function eve_api_characters_exist($characters) {
  $chars = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $result = db_query('SELECT characterID FROM {eve_api_characters} WHERE deleted = 0 AND characterID = :characterID', array(
        ':characterID' => (int) $character['characterID'],
      ));

      if ($result->rowCount()) {
        $chars[] = check_plain($character['characterName']);
      }
    }
  }

  if (empty($chars)) {
    $chars = FALSE;
  }

  return $chars;
}

/**
 * Function to check if the Characters is in a corp.
 *
 * @param int $character_id
 *   A unique character id.
 * @param int $corporation_id
 *   A unique corporation id.
 *
 * @return bool
 *   TRUE or FALSE.
 */
function eve_api_character_is_in_corp($character_id, $corporation_id = NULL) {
  $query = db_select('eve_api_character_roles', 'cr');

  if (isset($corporation_id)) {
    $query->join('eve_api_characters', 'c', 'cr.characterID = c.characterID');
    $query->condition('c.corporationID', $corporation_id, '=');
  }

  $result = $query->fields('cr', array(
      'characterID',
    ))
    ->condition('cr.roleID', 1, '=')
    ->condition('cr.characterID', (int) $character_id, '=')
    ->execute();

  if ($result->rowCount()) {
    return TRUE;
  }

  return FALSE;
}

/**
 * Function to check if the Characters is a Director.
 *
 * @param int $character_id
 *   A unique character id.
 * @param int $corporation_id
 *   A unique corporation id.
 *
 * @return bool
 *   TRUE or FALSE.
 */
function eve_api_character_is_director($character_id, $corporation_id = NULL) {
  $query = db_select('eve_api_character_roles', 'cr');

  if (isset($corporation_id)) {
    $query->join('eve_api_characters', 'c', 'cr.characterID = c.characterID');
    $query->condition('c.corporationID', $corporation_id, '=');
  }

  $result = $query->fields('cr', array(
      'characterID',
    ))
    ->condition('cr.roleID', 1, '=')
    ->condition('cr.characterID', (int) $character_id, '=')
    ->execute();

  if ($result->rowCount()) {
    return TRUE;
  }

  $query = db_select('eve_api_role_ceo_director', 'crw');

  if (isset($corporation_id)) {
    $query->join('eve_api_characters', 'c', 'crw.characterID = c.characterID');
    $query->condition('c.corporationID', $corporation_id, '=');
  }

  $result = $query->fields('crw', array(
      'characterID',
    ))
    ->condition('crw.isDirector', 1, '=')
    ->condition('crw.characterID', (int) $character_id, '=')
    ->execute();

  if ($result->rowCount()) {
    return TRUE;
  }

  return FALSE;
}

/**
 * Function to check if the Characters is a CEO.
 *
 * @param int $character_id
 *   A unique character id.
 * @param int $corporation_id
 *   A unique corporation id.
 *
 * @return bool
 *   TRUE or FALSE.
 */
function eve_api_character_is_ceo($character_id, $corporation_id = NULL) {
  $query = db_select('eve_api_alliance_corporations', 'ac');

  if (isset($corporation_id)) {
    $query->condition('ac.corporationID', $corporation_id, '=');
  }

  $result = $query->fields('ac', array(
      'ceoID',
    ))
    ->condition('ac.ceoID', (int) $character_id, '=')
    ->execute();

  if ($result->rowCount()) {
    return TRUE;
  }

  $query = db_select('eve_api_role_ceo_director', 'crw');

  if (isset($corporation_id)) {
    $query->join('eve_api_characters', 'c', 'crw.characterID = c.characterID');
    $query->condition('c.corporationID', $corporation_id, '=');
  }

  $result = $query->fields('crw', array(
      'characterID',
    ))
    ->condition('crw.isCEO', 1, '=')
    ->condition('crw.characterID', (int) $character_id, '=')
    ->execute();

  if ($result->rowCount()) {
    return TRUE;
  }

  return FALSE;
}

/**
 * Function that converts an Error ID to a readable message.
 *
 * @param int $error_id
 *   An error ID number.
 *
 * @return string
 *   An error message.
 */
function eve_api_api_error_msg($error_id) {
  switch ($error_id) {
    case 0:
      $message = t('OK');
      break;

    case 1:
      $message = t('Invalid API Mask/Type/Expiry');
      break;

    case 2:
      // Too Many Failed API Requests, we will wait 10 minutes to try again.
      $message = t('API Temporarily Down');
      break;

    case 3:
      // API Request Timed Out.
      $message = t('API Temporarily Down');
      break;

    case 4:
      // Invalid response, response received as a HTML document.
      $message = t('API Temporarily Down');
      break;

    case 5:
      // Invalid response cannot parse as XML.
      $message = t('API Temporarily Down');
      break;

    case 403:
      $message = t('API Key Does Not Exist');
      break;

    default:
      $message = t('API Temporarily Down');
      break;
  }

  return $message;
}

/**
 * Function that retrieves a simple list of API's associated with a user id.
 *
 * @param int $uid
 *   A unique Drupal User ID.
 *
 * @return mixed
 *   A simple array of character and api key information.
 */
function eve_api_list_api_simple($uid) {
  $uid = (int) $uid;
  $api_list = array();

  $result_a = db_query('SELECT apiID, keyID, vCode, errorID, updated, created FROM {eve_api_keys} WHERE uid = :uid', array(
    ':uid' => $uid,
  ));

  if ($result_a->rowCount()) {
    foreach ($result_a->fetchAll() as $row_a) {
      if ($row_a->errorID == 403) {
        continue;
      }

      $result_b = db_query('SELECT characterID, characterName, corporationID, corporationName, corporationTicker, allianceID, allianceName, allianceTicker FROM {eve_api_characters} WHERE deleted = 0 AND uid = :uid AND apiID = :apiID', array(
        ':uid' => $uid,
        ':apiID' => (int) $row_a->apiID,
      ));

      if ($result_b->rowCount()) {
        foreach ($result_b->fetchAll() as $row_b) {
          $api_list['characters'][$row_b->characterID] = array(
            'characterID' => check_plain($row_b->characterID),
            'characterName' => strip_tags(decode_entities($row_b->characterName)),
            'corporationID' => check_plain($row_b->corporationID),
            'corporationName' => strip_tags(decode_entities($row_b->corporationName)),
            'corporationTicker' => strip_tags(decode_entities($row_b->corporationTicker)),
            'allianceID' => check_plain($row_b->allianceID),
            'allianceName' => strip_tags(decode_entities($row_b->allianceName)),
            'allianceTicker' => strip_tags(decode_entities($row_b->allianceTicker)),
          );
        }
      }
    }
  }

  return $api_list;
}

/**
 * Function that retrieves a simple list of API's associated with a user id.
 *
 * @param object $account
 *   A Drupal user object containing.
 * @param int $key_id
 *   A unique EVE API Key ID.
 * @param string $v_code
 *   A unique EVE API Verification Code.
 *
 * @return mixed
 *   A mixed array of data regarding the success/failure.
 */
function eve_api_create_key($account, $key_id, $v_code) {
  $created = date('Y-m-d H:i:s', time());
  $uid = (int) $account->uid;

  $error_found = FALSE;
  $character_data = FALSE;

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = eve_api_get_character_api($query);

  $error = 0;

  if (isset($characters['error'])) {
    watchdog('eve_api', 'Error occured during registration. (' . drupal_http_build_query($query) . ')<br /><pre>' . print_r($characters, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);

    return FALSE;
  }

  if (!isset($characters['characters'])) {
    watchdog('eve_api', 'Error occured, should never happen. (' . drupal_http_build_query($query) . ')<br /><pre>' . print_r($characters, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);

    return FALSE;
  }

  if (isset($characters['errorCode'])) {
    $error = (int) $characters['errorCode'];
  }

  $whitelist = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $whitelist[] = (int) $character['characterID'];
    }
  }

  $result = db_query('SELECT characterID FROM {eve_api_whitelist} WHERE characterID IN (:characterIDs)', array(
    ':characterIDs' => $whitelist,
  ));

  $allow_expires = variable_get('eve_api_require_expires', FALSE) ? FALSE : !empty($characters['expires']);
  $allow_type = variable_get('eve_api_require_type', TRUE) ? $characters['type'] != 'Account' : FALSE;

  if ($result->rowCount()) {
    if ($allow_expires || ($characters['accessMask'] & 8388680) != 8388680) {
      $error = 1;
    }
  }
  else {
    if ($allow_expires || $allow_type || ($characters['accessMask'] & variable_get('eve_api_access_mask', 268435455)) != variable_get('eve_api_access_mask', 268435455)) {
      $error = 1;
    }
  }

  $api_id = db_insert('eve_api_keys')->fields(array(
    'uid' => (int) $uid,
    'keyID' => (int) $key_id,
    'vCode' => (string) $v_code,
    'updated' => (string) $created,
    'created' => (string) $created,
    'errorID' => (int) $error,
  ))->execute();

  if ($error == 1) {
    return FALSE;
  }

  foreach ($characters['characters'] as $character) {
    db_merge('eve_api_characters')->key(array('characterID' => (int) $character['characterID']))->fields(array(
      'characterID' => (int) $character['characterID'],
      'uid' => (int) $uid,
      'apiID' => (int) $api_id,
      'characterName' => (string) $character['characterName'],
      'corporationID' => (int) $character['corporationID'],
      'corporationName' => (string) $character['corporationName'],
      'corporationTicker' => (string) $character['corporationTicker'],
      'allianceID' => (int) $character['allianceID'],
      'allianceName' => (string) $character['allianceName'],
      'allianceTicker' => (string) $character['allianceTicker'],
      'errorID' => (int) $error,
      'deleted' => 0,
    ))->execute();

    if ($account->name == (string) $character['characterName']) {
      $character_data = $character;
    }
  }

  return isset($character_data) ? $character_data : array('not_found' => TRUE);
}

/**
 * Function that retrieves a simple list of API's associated with a user id.
 *
 * @param object $account
 *   A Drupal user object containing.
 * @param int $key_id
 *   A unique EVE API Key ID.
 * @param string $v_code
 *   A unique EVE API Verification Code.
 * @param int $api_id
 *   A unique API ID.
 *
 * @return mixed
 *   A mixed array of data regarding the success/failure.
 */
function eve_api_modify_key($account, $key_id, $v_code, $api_id) {
  $updated = date('Y-m-d H:i:s', time());
  $uid = (int) $account->uid;

  $character_found = FALSE;
  $error_found = FALSE;
  $character_data = FALSE;

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = eve_api_get_character_api($query);

  $error = 0;

  if (isset($characters['error'])) {
    if ($api_id == 0) {
      return array(
        'error' => TRUE,
        'error_found' => $error_found,
        'character_data' => $character_data,
        'character_found' => $character_found,
      );
    }

    db_merge('eve_api_keys')->key(array('apiID' => (int) $api_id))->fields(array(
      'apiID' => (int) $api_id,
      'errorID' => (int) $characters['error'],
      'updated' => (string) $updated,
    ))->execute();

    if ((int) $characters['error'] == 403) {
      db_update('eve_api_characters')->fields(array('errorID' => (int) $characters['error'], 'deleted' => 1))->condition('apiID', (int) $api_id, '=')->execute();
    }
    else {
      db_update('eve_api_characters')->fields(array('errorID' => (int) $characters['error']))->condition('apiID', (int) $api_id, '=')->execute();
      $error_found = TRUE;
    }

    return array(
      'error' => FALSE,
      'error_found' => $error_found,
      'character_data' => $character_data,
      'character_found' => $character_found,
    );
  }

  if (!isset($characters['characters'])) {
    watchdog('eve_api', 'Error occured, should never happen. (' . drupal_http_build_query($query) . ')<br /><pre>' . print_r($characters, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);

    return array(
      'error' => TRUE,
      'error_found' => $error_found,
      'character_data' => $character_data,
      'character_found' => $character_found,
    );
  }

  if (isset($characters['errorCode'])) {
    $error = (int) $characters['errorCode'];
  }

  $whitelist = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $whitelist[] = (int) $character['characterID'];
    }
  }

  $result = db_query('SELECT characterID FROM {eve_api_whitelist} WHERE characterID IN (:characterIDs)', array(
    ':characterIDs' => $whitelist,
  ));

  $allow_expires = variable_get('eve_api_require_expires', FALSE) ? FALSE : !empty($characters['expires']);
  $allow_type = variable_get('eve_api_require_type', TRUE) ? $characters['type'] != 'Account' : FALSE;

  if ($result->rowCount()) {
    if ($allow_expires || ($characters['accessMask'] & 8388680) != 8388680) {
      $error = 1;
    }
  }
  else {
    if ($allow_expires || $allow_type || ($characters['accessMask'] & variable_get('eve_api_access_mask', 268435455)) != variable_get('eve_api_access_mask', 268435455)) {
      $error = 1;
    }
  }

  db_update('eve_api_characters')->fields(array('errorID' => (int) $error, 'deleted' => 1))->condition('apiID', (int) $api_id)->execute();
  db_merge('eve_api_keys')->key(array('apiID' => (int) $api_id))->fields(array(
    'apiID' => (int) $api_id,
    'uid' => (int) $uid,
    'keyID' => (int) $key_id,
    'vCode' => (string) $v_code,
    'updated' => (string) $updated,
    'errorID' => (int) $error,
  ))->execute();

  if ($error == 1) {
    return array(
      'error' => FALSE,
      'error_found' => $error_found,
      'character_data' => $character_data,
      'character_found' => $character_found,
    );
  }

  foreach ($characters['characters'] as $character) {
    $error = 0;

    if (isset($characters['errorCode'])) {
      $error = (int) $characters['errorCode'];
    }

    db_merge('eve_api_characters')->key(array('characterID' => (int) $character['characterID']))->fields(array(
      'characterID' => (int) $character['characterID'],
      'uid' => (int) $uid,
      'apiID' => (int) $api_id,
      'characterName' => (string) $character['characterName'],
      'corporationID' => (int) $character['corporationID'],
      'corporationName' => (string) $character['corporationName'],
      'corporationTicker' => (string) $character['corporationTicker'],
      'allianceID' => (int) $character['allianceID'],
      'allianceName' => (string) $character['allianceName'],
      'allianceTicker' => (string) $character['allianceTicker'],
      'errorID' => (int) $error,
      'deleted' => 0,
    ))->execute();

    if ($account->name == (string) $character['characterName']) {
      $character_found = TRUE;
      $character_data = $character;
    }
  }

  return array(
    'error' => FALSE,
    'error_found' => $error_found,
    'character_data' => $character_data,
    'character_found' => $character_found,
  );
}

/**
 * Function that retrieves a simple list of API's associated with a user id.
 *
 * @param object $account
 *   A Drupal user object containing.
 * @param int $key_id
 *   A unique EVE API Key ID.
 * @param string $v_code
 *   A unique EVE API Verification Code.
 * @param int $api_id
 *   A unique API ID.
 *
 * @return mixed
 *   A mixed array of data regarding the success/failure.
 */
function eve_api_update_key($account, $key_id, $v_code, $api_id) {
  $updated = date('Y-m-d H:i:s', time());
  $uid = (int) $account->uid;

  $character_found = FALSE;
  $error_found = FALSE;
  $character_data = FALSE;

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = eve_api_get_character_api($query);

  $error = 0;

  if (isset($characters['error'])) {
    if ($api_id == 0) {
      return array(
        'error' => TRUE,
        'error_found' => $error_found,
        'character_data' => $character_data,
        'character_found' => $character_found,
      );
    }

    db_merge('eve_api_keys')->key(array('apiID' => (int) $api_id))->fields(array(
      'apiID' => (int) $api_id,
      'errorID' => (int) $characters['error'],
      'updated' => (string) $updated,
    ))->execute();

    if ((int) $characters['error'] == 403) {
      db_update('eve_api_characters')->fields(array('errorID' => (int) $characters['error'], 'deleted' => 1))->condition('apiID', (int) $api_id, '=')->execute();
    }
    else {
      db_update('eve_api_characters')->fields(array('errorID' => (int) $characters['error']))->condition('apiID', (int) $api_id, '=')->execute();
      $error_found = TRUE;
    }

    return array(
      'error' => FALSE,
      'error_found' => $error_found,
      'character_data' => $character_data,
      'character_found' => $character_found,
    );
  }

  if (!isset($characters['characters'])) {
    watchdog('eve_api', 'Error occured, should never happen. (' . drupal_http_build_query($query) . ')<br /><pre>' . print_r($characters, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);

    return array(
      'error' => TRUE,
      'error_found' => $error_found,
      'character_data' => $character_data,
      'character_found' => $character_found,
    );
  }

  if (isset($characters['errorCode'])) {
    $error = (int) $characters['errorCode'];
  }

  $whitelist = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $whitelist[] = (int) $character['characterID'];
    }
  }

  $result = db_query('SELECT characterID FROM {eve_api_whitelist} WHERE characterID IN (:characterIDs)', array(
    ':characterIDs' => $whitelist,
  ));

  $allow_expires = variable_get('eve_api_require_expires', FALSE) ? FALSE : !empty($characters['expires']);
  $allow_type = variable_get('eve_api_require_type', TRUE) ? $characters['type'] != 'Account' : FALSE;

  if ($result->rowCount()) {
    if ($allow_expires || ($characters['accessMask'] & 8388680) != 8388680) {
      $error = 1;
    }
  }
  else {
    if ($allow_expires || $allow_type || ($characters['accessMask'] & variable_get('eve_api_access_mask', 268435455)) != variable_get('eve_api_access_mask', 268435455)) {
      $error = 1;
    }
  }

  db_update('eve_api_characters')->fields(array('errorID' => (int) $error, 'deleted' => 1))->condition('apiID', (int) $api_id)->execute();
  db_merge('eve_api_keys')->key(array('apiID' => (int) $api_id))->fields(array(
    'apiID' => (int) $api_id,
    'uid' => (int) $uid,
    'keyID' => (int) $key_id,
    'vCode' => (string) $v_code,
    'updated' => (string) $updated,
    'errorID' => (int) $error,
  ))->execute();

  if ($error == 1) {
    return array(
      'error' => FALSE,
      'error_found' => $error_found,
      'character_data' => $character_data,
      'character_found' => $character_found,
    );
  }

  foreach ($characters['characters'] as $character) {
    $error = 0;

    if (isset($characters['errorCode'])) {
      $error = (int) $characters['errorCode'];
    }

    db_merge('eve_api_characters')->key(array('characterID' => (int) $character['characterID']))->fields(array(
      'characterID' => (int) $character['characterID'],
      'uid' => (int) $uid,
      'apiID' => (int) $api_id,
      'characterName' => (string) $character['characterName'],
      'corporationID' => (int) $character['corporationID'],
      'corporationName' => (string) $character['corporationName'],
      'corporationTicker' => (string) $character['corporationTicker'],
      'allianceID' => (int) $character['allianceID'],
      'allianceName' => (string) $character['allianceName'],
      'allianceTicker' => (string) $character['allianceTicker'],
      'errorID' => (int) $error,
      'deleted' => 0,
    ))->execute();

    if ($account->name == (string) $character['characterName']) {
      $character_found = TRUE;
      $character_data = $character;
    }

    $sheet_query = $query + array('characterID' => (int) $character['characterID']);
    $character_sheet = eve_api_get_character_sheet_api($sheet_query);
    $error = 0;

    if (isset($character_sheet['errorCode'])) {
      $error = (int) $character_sheet['errorCode'];
    }

    if (isset($character_sheet['error'])) {
      $error = (int) $character_sheet['error'];

      db_merge('eve_api_character_sheet')->key(array(
        'characterID' => (int) $character['characterID'],
      ))->fields(array(
        'characterID' => (int) $character['characterID'],
        'errorID' => (int) $error,
      ))->execute();
      continue;
    }
    else {
      db_delete('eve_api_character_sheet_attribute_enhancers')->condition('characterID', (int) $character['characterID'], '=')->execute();
      db_delete('eve_api_character_roles')->condition('characterID', (int) $character['characterID'], '=')->execute();
      db_delete('eve_api_character_sheet_titles')->condition('characterID', (int) $character['characterID'], '=')->execute();
    }

    foreach ($character_sheet as $type => $data) {
      switch ((string) $type) {
        case 'skills':
          foreach ($data as $skillsdata) {
            db_merge('eve_api_character_sheet_skills')->key(array(
              'characterID' => (int) $character['characterID'],
              'typeID' => (int) $skillsdata['typeID'],
            ))->fields(array(
              'characterID' => (int) $character['characterID'],
              'typeID' => (int) $skillsdata['typeID'],
              'skillpoints' => (int) $skillsdata['skillpoints'],
              'level' => (int) $skillsdata['level'],
              'published' => (int) $skillsdata['published'],
            ))->execute();
          }
          break;

        case 'certificates':
          foreach ($data as $certificatesdata) {
            db_merge('eve_api_character_sheet_certificates')->key(array(
              'characterID' => (int) $character['characterID'],
              'certificateID' => (int) $certificatesdata['certificateID'],
            ))->fields(array(
              'characterID' => (int) $character['characterID'],
              'certificateID' => (int) $certificatesdata['certificateID'],
            ))->execute();
          }
          break;

        case 'corporationTitles':
          foreach ($data as $titlesdata) {
            db_merge('eve_api_character_sheet_titles')->key(array(
              'characterID' => (int) $character['characterID'],
              'titleID' => (int) $titlesdata['titleID'],
            ))->fields(array(
              'characterID' => (int) $character['characterID'],
              'titleID' => (int) $titlesdata['titleID'],
              'titleName' => (int) $titlesdata['titleName'],
            ))->execute();
          }
          break;

        case 'roles':
          foreach ($data as $rolename => $roledata) {
            foreach ($roledata as $roles) {
              db_merge('eve_api_character_roles')->key(array(
                'characterID' => (int) $character['characterID'],
                'type' => (string) $rolename,
                'roleID' => (int) $roles['roleID'],
              ))->fields(array(
                'characterID' => (int) $character['characterID'],
                'type' => (string) $rolename,
                'roleID' => (int) $roles['roleID'],
                'roleName' => (string) $roles['roleName'],
              ))->execute();
            }
          }
          break;

        case 'attributeEnhancers':
          foreach ($data as $attributename => $attributedata) {
            $query_data = db_insert('eve_api_character_sheet_attribute_enhancers');

            $fields = array(
              'characterID' => (int) $character['characterID'],
              'bonusName' => (string) $attributename,
            );

            foreach ($attributedata as $attribute => $attributes) {
              $fields += array(
                (string) $attribute => (string) $attributes,
              );
            }

            $query_data->fields($fields);
            $query_data->execute();
          }
          break;

        case 'info':
          $query_data = db_merge('eve_api_character_sheet')->key(array(
            'characterID' => (int) $character['characterID'],
          ));

          $fields = array(
            'characterID' => (int) $character['characterID'],
            'errorID' => (int) $error,
          );

          $ignored = array(
            'corporationName',
            'corporationID',
            'allianceName',
            'allianceID',
            'name',
          );

          foreach ($data as $infoname => $infodata) {
            if (in_array($infoname, $ignored)) {
              continue;
            }

            $fields += array(
              (string) $infoname => (string) $infodata,
            );
          }

          $query_data->fields($fields);
          $query_data->execute();
          break;

        default:
          break;
      }
    }

    $info_query = $query + array('characterID' => (int) $character['characterID']);
    $character_info = eve_api_get_character_info_api($info_query);
    $error = 0;

    if (isset($character_info['errorCode'])) {
      $error = (int) $character_info['errorCode'];
    }

    if (isset($character_info['error'])) {
      $error = (int) $character_info['error'];

      db_merge('eve_api_character_info')->key(array(
        'characterID' => (int) $character['characterID'],
      ))->fields(array(
        'characterID' => (int) $character['characterID'],
        'errorID' => (int) $error,
      ))->execute();
      continue;
    }
    else {
      db_delete('eve_api_character_info_corporation_history')->condition('characterID', (int) $character['characterID'], '=')->execute();
    }

    foreach ($character_info as $type => $data) {
      switch ((string) $type) {
        case 'employmentHistory':
          foreach ($data as $historydata) {
            db_merge('eve_api_character_info_corporation_history')->key(array(
              'characterID' => (int) $character['characterID'],
              'recordID' => (int) $historydata['recordID'],
            ))->fields(array(
              'characterID' => (int) $character['characterID'],
              'recordID' => (int) $historydata['recordID'],
              'corporationID' => (int) $historydata['corporationID'],
              'startDate' => (string) $historydata['startDate'],
            ))->execute();
          }
          break;

        case 'info':
          $query_data = db_merge('eve_api_character_info')->key(array(
            'characterID' => (int) $character['characterID'],
          ));

          $fields = array(
            'characterID' => (int) $character['characterID'],
            'errorID' => (int) $error,
          );

          $ignored = array(
            'characterID',
          );

          foreach ($data as $infoname => $infodata) {
            if (in_array($infoname, $ignored)) {
              continue;
            }

            $fields += array(
              (string) $infoname => (string) $infodata,
            );
          }

          $query_data->fields($fields);
          $query_data->execute();
          break;

        default:
          break;
      }
    }
  }

  return array(
    'error' => FALSE,
    'error_found' => $error_found,
    'character_data' => $character_data,
    'character_found' => $character_found,
  );
}
