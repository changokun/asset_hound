<?php
class asset_hound {

    private static $assets = array(); // this collects asset objects over the course of request processing.
    private static $buffers = array(); // this is used to record added assets during a section of code. see example at container processing.
    public static $asset_hound_output_format = 'conventional HTML script tags';
    public static $asset_classes_in_order = array('Internet Explorer fix', 'super library', 'library', 'framework', 'standard');

    private function __construct() {}

    public static function add($assets) { // facade accepts asset ot array thereof
        if(is_array($assets)) {
            foreach($assets as $asset) self::_add($asset);
        } else {
            self::_add($assets);
        }
    }

    private static function _add(asset $asset) {
        // obv, add it to the pile
        self::$assets[] = $asset;
        // but also add it to any listening buffers
        foreach(self::$buffers as $key => $buffer) self::$buffers[$key][] = $asset;
    }

    static function get_final_output() {
        // groom, remove duplicates and blanks.
        $already_have = array();
        foreach(self::$assets as $key => $asset) {
            if( ! strlen($asset->js_code) and ! strlen($asset->css_code)) {
                if(in_array($asset->file_name, $already_have)) {
                    unset(self::$assets[$key]);
                    if(m::help('asset', 3)) decho('not a problem at all; ' . $asset->file_name . ' has been added to ' . __CLASS__ . ' more than once.');
                }
                if( ! strlen($asset->file_name) and ! strlen($asset->js_code) and ! strlen($asset->css_code)) unset(self::$assets[$key]); // todo add alert on help
                $already_have[] = $asset->file_name;
            }
        }
        unset($already_have);

        // if help is turned on, we will look for assets whose files do not exist.
        if(m::help('asset')) {
            foreach(self::$assets as $asset) {
                if(strlen($asset->file_name)) {
                    if(
                        (substr($asset->file_name, 0, 1) == '/' and ! @file_exists($_SERVER['DOCUMENT_ROOT'] . $asset->file_name))
                        or (substr($asset->file_name, 0, 4) == 'http' and ! @file_get_contents($asset->file_name))
                    ) {
                        m::decho('<h3 style="color:crimson">OH NO!</h3>' . $asset->file_name . ' has been added to ' . __CLASS__ . ', but the source file does not exist!<br>this is a problem because we do not check for file existence on every run. please put the file in place, or stop adding it to ' . __CLASS__ . '. ' . implode(' | ', $asset->note));
                    }
                }
            }
        }

        if(self::$asset_hound_output_format == 'conventional HTML script tags') {
            return self::get_tags();
        } else {
            // do your alternate development here. well, in another func, but call it from here.
            throw new Exception('i do not know how to handle ');
        }
    }

    private static function get_tags() {

        // get them into proper order!
        self::sort_assets();

        //m::death(self::$assets, 'sorted assets');
        // now create the output - i suppose we could ask each asset to run its own __toString.....hmmmm - but that is not clear enough - would you get html or yepnope or what?
        $output = array();

        foreach(self::$assets as $asset) {
            $tag = '';
            if( ! empty($asset->media)) $asset->media = 'media="' . $asset->media . '" ';
            if( ! empty($asset->conditional)) $tag .= '<!--[' . $asset->conditional . ']>';
            if( ! empty($asset->js_code)) {
                $tag .= '<script type="text/javascript">' . $asset->js_code . '</script>';
            } elseif( ! empty($asset->css_code)) {
                $tag .= '<style type="text/css" ' . $asset->media . '>' . $asset->css_code . '</style>';
            } elseif(stripos($asset->file_name, 'js') !== false) {
                $tag .= '<script type="text/javascript" src="' . $asset->file_name . '"></script>';
            } else { // the css assets
                $tag .= '<link rel="stylesheet" type="text/css" href="' . $asset->file_name . '" ' . $asset->media . '/>';
            }
            if( ! empty($asset->conditional)) $tag .= '<![endif]-->';

            if(m::help('asset')) {
                array_unshift($asset->note, $asset->class);
                if(isset($asset->note) and count($asset->note)) $tag .= '<!-- ' . implode(' | ', $asset->note) . ' -->';
            }

            $output[] = $tag;
        }

        $output = "\n<!-- asset hound -->\n" . implode("\n", $output) . "\n\n";

        return $output;
    }

    private static function sort_assets() {
        usort(self::$assets, array('self', '_sort_assets_compare'));
    }

    private static function _sort_assets_compare($a, $b) { // do not use this method outside of sort_assets()
        // put ie fixes first
        $a_is_ie_fix = $a->class == 'ie_fix' ? 1 : 0;
        $b_is_ie_fix = $b->class == 'ie_fix' ? 1 : 0;
        if($a_is_ie_fix == $b_is_ie_fix) {
            // now put css first.
            $a_is_css = stripos($a->file_name, '.css') !== false ? 1 : 0;
            $b_is_css = stripos($b->file_name, '.css') !== false ? 1 : 0;
            if($a_is_css === $b_is_css) {
                // now sort by class
                if($a->class === $b->class) {
                    // now put conditionals at the end
                    return strcmp($a->conditional, $b->conditional); // should put them in ascending order of ie versions, and at the end of the class section.
                } else {
                    return array_search($a->class, self::$asset_classes_in_order) > array_search($b->class, self::$asset_classes_in_order) ? 1 : -1;
                }
            } else {
                return $a_is_css < $b_is_css ? 1 : -1; // this should put all the css first.
            }
        } else {
            return $a_is_ie_fix < $b_is_ie_fix ? 1 : -1; // this should put all the ie_fixes first.
        }
    }

    public static function buffer_start($buffer_key) {
        self::$buffers[$buffer_key] = array(); // will erase any existing buffer. perhaps this should be an option/arg.
    }

    public static function buffer_get_clean($buffer_key) {
        //m::dump(self::$buffers);
        $ret = array();
        if(isset(self::$buffers[$buffer_key])) {
            $ret = self::$buffers[$buffer_key];
            unset(self::$buffers[$buffer_key]);
        }
        return $ret;
    }

}













class asset {

    static $asset_classes;
    static $asset_keywords;
    private static $assets_by_keyword; // will be something like array('jQuery' => {serialized asset?}); // should attempt to cache.

    function __construct($file_name = '', $class = '', $media = '', $conditional = '', $js_code = '', $css_code = '', $note = array()) {
        // i do not accept keywords. use asset::get_asset_by_keyword()

        // do we have a valid asset?
        if(empty($file_name) and empty($js_code) and empty($css_code)) return false;

        // the real default class is 'standard' - yes, this must be applied so that global_actions can come last.
        if(empty($class)) $class = 'standard';

        // good to specify these, i guess.
        if(empty($media)) $media = 'screen, projection';

        // todo add any checking we want about jquery being lib level stuff
        // todo? if the filename contains _ie6 and no test supplied, auto add teh ie6 test. etc

        // if the file name doesn't begin with http, make sure it starts with a slash
        if(strlen($file_name) and substr($file_name, 0, 1) != '/' and substr($file_name, 0, 4) != 'http') $file_name = '/' . $file_name;
        $this->file_name = $file_name;
        $this->class = $class; // todo check that it is a valid class? nah. maybe on help. yeah. todo
        $this->media = $media;
        $this->conditional = $conditional;
        $this->js_code = $js_code;
        $this->css_code = $css_code;
        if( ! is_array($note)) $note = array(trim($note));
        $this->note = $note;

        return $this;
    }

    // we'll put asset keywords and their definitions in an ini file. how does that sound? modify this to pull from any source youlike.
    static function get_asset_keywords() {
        if(isset(self::$asset_keywords)) return self::$asset_keywords;
        self::$asset_keywords = array();
        //todo
        m::death(self::$asset_keywords, '$asset_keywords');
        return self::$asset_keywords;
    }



    // return asset object, or false on error.
    static function get_asset_by_keyword($keyword = '') {
        if(empty(self::$assets_by_keyword)) self::load_asset_keywords();
        //m::death(self::$assets_by_keyword, 'all the known keywords for assets.');
        // obviously, keywords cannot contain '.js' or '.css' etc
        if(isset(self::$assets_by_keyword[$keyword])) return self::$assets_by_keyword[$keyword];
        // keyword not found - need to be silent, but should warn
        trigger_error('asset keyword ' . $keyword . ' not found', E_USER_WARNING);
        return array();
    }

    private static function load_asset_keywords() {
        // todo - this data will probably be read from a db or ini file, and since it involves waking serialized asset_objects. we should cache it somewhere. memcache?
        // for now, create an array of asset objects
        // todo - these need to come from sarumino.com but there is dns waiting issues.
        self::$assets_by_keyword = array(
            'mBox' => array(
                new asset('http://www.levitonpress.com/public/assets/lib/mBox/mBox.js'),
                // add your own extension. jeez. new asset('http://www.levitonpress.com/clients/lib/mBox/mBox_extension.js'),
                new asset('http://www.levitonpress.com/public/assets/lib/mBox/mBox.css')
            ),
            'term_browse' => array(
                new asset('/public/assets/lib/xio/term_browse/term_browse.js'),
                new asset('/public/assets/lib/xio/term_browse/term_browse.css')
            ),
            'jQuery_1.7' => array(
                new asset('https://ajax.googleapis.com/ajax/libs/jquery/1.7/jquery.min.js', 'super library')
            ),
            'skeleton_1.2' => array(
                new asset('http://www.levitonpress.com/public/assets/lib/Skeleton/1.2/base.css', 'framework'),
                new asset('http://www.levitonpress.com/public/assets/lib/Skeleton/1.2/layout.css', 'framework'),
                new asset('http://www.levitonpress.com/public/assets/lib/Skeleton/1.2/skeleton.css', 'framework')
            )
        );
    }

}

