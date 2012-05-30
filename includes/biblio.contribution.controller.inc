<?php

/**
 * Contribution class.
 */
class BiblioContribution extends Entity {
  protected function defaultUri() {
    return array('path' => 'biblio/contribution/' . $this->identifier());
  }
}


/**
 * Contribution controller class 
 */
class BiblioContributionController extends EntityAPIController {
  /**
   * Creates default property values for our entity object
   * 
   * @global type $user
   * @param array $values
   * @return array 
   */
  public function create(array $values = array()) {
    global $user;
    $values += array(
      'cnid' => '',
      'type' => 'contribution',
      'rank' => 0,
      'created' => REQUEST_TIME,
      'changed' => REQUEST_TIME,
    );
    return parent::create($values);
  }
}