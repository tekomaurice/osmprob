#' Convert graph stored in \code{data.frame} to \code{sf}
#'
#' @param dat \code{data.frame} containing graph data.
#'
#' @noRd
get_graph <- function (dat)
{
    dat$from_lat %<>% as.character %>% as.numeric
    dat$from_lon %<>% as.character %>% as.numeric
    dat$to_lat %<>% as.character %>% as.numeric
    dat$to_lon %<>% as.character %>% as.numeric

    from <- cbind (dat$from_lon, dat$from_lat)
    to <- cbind (dat$to_lon, dat$to_lat)
    fromTo <- cbind (from, to)
    graphLines <- list ("LINESTRING", dim (fromTo) [1])
    for (i in 1:dim (fromTo) [1])
    {
        pair <- fromTo [i,]
        graphLines [[i]] <- sf::st_linestring (rbind (c (pair [1], pair [2]),
                                                      c (pair [3], pair [4])))
    }

    graph <- sf::st_sfc (graphLines, crs = 4326)
    lt_ln <- c ("from_lat", "from_lon", "to_lat", "to_lon")
    dat [lt_ln] <- NULL
    graph <- sf::st_sf (graph, dat)

    graph
}

#' Select vertices on graph that are closest to the specified coordinates.
#'
#' @param graph \code{data.frame} containing the street network.
#' @param start_coords \code{numeric} coordinates of the start point.
#' @param end_coords \code{numeric} coordinates of the end point.
#'
#' @return \code{list} containing the two rows of the input \code{data.frame}
#' that are closest to the start and end coordinates
#'
#' @export
select_vertices_by_coordinates <- function (graph, start_coords, end_coords)
{
    com <- graph$compact
    d_start <- sqrt ((start_coords [1] - com$from_lon)^2 +
                     (start_coords [2] - com$from_lat)^2)
    d_end <- sqrt ((end_coords [1] - com$to_lon)^2 +
                     (end_coords [2] - com$to_lat)^2)

    st_index <- which.min (d_start)
    en_index <- which.min (d_end)
    start_id <- com [st_index, "from_id"] %>% as.character
    end_id <- com [en_index, "to_id"] %>% as.character
    c (start_id, end_id)
}