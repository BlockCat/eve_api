<?php

/**
 * Function to retrieve the standings for a cron job
 */
function omni_eve_api_get_standings_cron(&$xml, $keyid, $vcode)
{
  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );
  omni_eve_api_get_xml($xml, 'standings', $query);

  if (!empty($xml -> result -> rowset[0]))
  {
    foreach ($xml->result->rowset[0] as $s)
    {
      $cstandings[(string)$s["contactID"]] = array(
        (string)$s["contactName"],
        (string)$s["standing"]
      );
    }
  }

  if (!empty($xml -> result -> rowset[1]))
  {
    foreach ($xml->result->rowset[1] as $s)
    {
      $astandings[(string)$s["contactID"]] = array(
        (string)$s["contactName"],
        (string)$s["standing"]
      );
    }
  }

  $count = 0;

  if (!empty($cstandings))
  {
    foreach ($cstandings as $i => $c)
    {
      if ($c[1] > 0)
      {
        $blues[$i][0] = $c[0];
        $blues[$i][1] = $c[1];
        $blues[$i][2] = 0;
        $count++;
      }
      elseif ($c[1] < 0)
      {
        $reds[$i][0] = $c[0];
        $reds[$i][1] = $c[1];
        $reds[$i][2] = 0;
        $count++;
      }
    }
  }

  if (!empty($astandings))
  {
    foreach ($astandings as $i => $a)
    {
      if ($a[1] > 0)
      {
        $blues[$i][0] = $a[0];
        $blues[$i][2] = $a[1];
        $count++;

        if (!isset($blues[$i][1]))
        {
          $blues[$i][1] = 0;
        }
      }
      elseif ($a[1] < 0)
      {
        $reds[$i][0] = $a[0];
        $reds[$i][2] = $a[1];
        $count++;

        if (!isset($reds[$i][1]))
        {
          $reds[$i][1] = 0;
        }
      }
    }
  }

  return array(
    $blues,
    $reds,
    $count,
  );
}
