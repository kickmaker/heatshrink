#ifndef HEATSHRINK_DECODER_H
#define HEATSHRINK_DECODER_H

#include <stdint.h>
#include <stddef.h>
#include "heatshrink_common.h"

#if CONFIG_ZEPHYR_HEATSHRINK_MODULE
#include "zephyr/heatshrink_config.h"
#else
#include "heatshrink_config.h"
#endif

/**
 * @file heatshrink_decoder.h
 *
 * @brief Heatshrink Decoder API
 *
 * This header file defines the API for the Heatshrink decoder library.
 */

/**
 * @brief Sink result codes.
 *
 * These are the possible return values for the `heatshrink_decoder_sink` function.
 */
typedef enum {
    HSDR_SINK_OK,               /**< Data sunk successfully, ready to poll. */
    HSDR_SINK_FULL,             /**< Out of space in the internal buffer. */
    HSDR_SINK_ERROR_NULL = -1   /**< NULL argument error. */
} HSD_sink_res;

/**
 * @brief Poll result codes.
 *
 * These are the possible return values for the `heatshrink_decoder_poll` function.
 */
typedef enum {
    HSDR_POLL_EMPTY,            /**< Input exhausted. */
    HSDR_POLL_MORE,             /**< More data remaining, call again with a fresh output buffer. */
    HSDR_POLL_ERROR_NULL = -1,  /**< NULL argument error. */
    HSDR_POLL_ERROR_UNKNOWN = -2 /**< Unknown error. */
} HSD_poll_res;

/**
 * @brief Finish result codes.
 *
 * These are the possible return values for the `heatshrink_decoder_finish` function.
 */
typedef enum {
    HSDR_FINISH_DONE,           /**< Output is done. */
    HSDR_FINISH_MORE,           /**< More output remains. */
    HSDR_FINISH_ERROR_NULL = -1  /**< NULL argument error. */
} HSD_finish_res;

#if HEATSHRINK_DYNAMIC_ALLOC
#define HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(BUF) \
    ((BUF)->input_buffer_size)
#define HEATSHRINK_DECODER_WINDOW_BITS(BUF) \
    ((BUF)->window_sz2)
#define HEATSHRINK_DECODER_LOOKAHEAD_BITS(BUF) \
    ((BUF)->lookahead_sz2)
#else
#define HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(_) \
    HEATSHRINK_STATIC_INPUT_BUFFER_SIZE
#define HEATSHRINK_DECODER_WINDOW_BITS(_) \
    (HEATSHRINK_STATIC_WINDOW_BITS)
#define HEATSHRINK_DECODER_LOOKAHEAD_BITS(BUF) \
    (HEATSHRINK_STATIC_LOOKAHEAD_BITS)
#endif

/**
 * @brief Heatshrink decoder structure.
 *
 * This structure represents a Heatshrink decoder. It contains various fields
 * necessary for the decoding process.
 */
typedef struct {
    uint16_t input_size;        /**< Bytes in the input buffer. */
    uint16_t input_index;       /**< Offset to the next unprocessed input byte. */
    uint16_t output_count;      /**< How many bytes to output. */
    uint16_t output_index;      /**< Index for bytes to output. */
    uint16_t head_index;        /**< Head of the window buffer. */
    uint8_t state;              /**< Current state machine node. */
    uint8_t current_byte;       /**< Current byte of input. */
    uint8_t bit_index;          /**< Current bit index. */

#if HEATSHRINK_DYNAMIC_ALLOC
    /* Fields that are only used if dynamically allocated. */
    uint8_t window_sz2;         /**< Window buffer bits. */
    uint8_t lookahead_sz2;      /**< Lookahead bits. */
    uint16_t input_buffer_size; /**< Input buffer size. */

    /* Input buffer, then expansion window buffer. */
    uint8_t buffers[];
#else
    /* Input buffer, then expansion window buffer. */
    uint8_t buffers[(1 << HEATSHRINK_DECODER_WINDOW_BITS(_))
        + HEATSHRINK_DECODER_INPUT_BUFFER_SIZE(_)];
#endif
} heatshrink_decoder;

#if HEATSHRINK_DYNAMIC_ALLOC
/**
 * @brief Allocate a Heatshrink decoder.
 * @note Only for dynamic allocation mode
 *
 * Allocate a decoder with an input buffer of INPUT_BUFFER_SIZE bytes, an expansion
 * buffer size of 2^WINDOW_SZ2, and a lookahead size of 2^LOOKAHEAD_SZ2.
 *
 * @param input_buffer_size Size of the input buffer.
 * @param expansion_buffer_sz2 Size of the expansion buffer (window size).
 * @param lookahead_sz2 Lookahead size.
 *
 * @return Pointer to the allocated decoder, or NULL on error.
 */
heatshrink_decoder *heatshrink_decoder_alloc(uint16_t input_buffer_size,
    uint8_t expansion_buffer_sz2, uint8_t lookahead_sz2);

/**
 * @brief Free a Heatshrink decoder.
 * @note Only for dynamic allocation mode
 *
 * Free the memory associated with a decoder.
 *
 * @param hsd Pointer to the decoder to be freed.
 */
void heatshrink_decoder_free(heatshrink_decoder *hsd);
#endif /* HEATSHRINK_DYNAMIC_ALLOC */

/**
 * @brief Reset a Heatshrink decoder.
 *
 * Reset a decoder to its initial state.
 *
 * @param hsd Pointer to the decoder to be reset.
 */
void heatshrink_decoder_reset(heatshrink_decoder *hsd);

/**
 * @brief Sink data into the decoder.
 *
 * Sink at most SIZE bytes from IN_BUF into the decoder. The INPUT_SIZE is set to
 * indicate how many bytes were actually sunk (in case a buffer was filled).
 *
 * @param hsd Pointer to the decoder.
 * @param in_buf Pointer to the input buffer.
 * @param size Size of the input data.
 * @param input_size Pointer to the variable indicating the number of bytes actually sunk.
 *
 * @return HSD_sink_res value indicating the result of the operation.
 */
HSD_sink_res heatshrink_decoder_sink(heatshrink_decoder *hsd,
    uint8_t *in_buf, size_t size, size_t *input_size);

/**
 * @brief Poll for output from the decoder.
 *
 * Poll for output from the decoder, copying at most OUT_BUF_SIZE bytes into
 * OUT_BUF (setting OUTPUT_SIZE to the actual amount copied).
 *
 * @param hsd Pointer to the decoder.
 * @param out_buf Pointer to the output buffer.
 * @param out_buf_size Size of the output buffer.
 * @param output_size Pointer to the variable indicating the number of bytes actually output.
 *
 * @return HSD_poll_res value indicating the result of the operation.
 */
HSD_poll_res heatshrink_decoder_poll(heatshrink_decoder *hsd,
    uint8_t *out_buf, size_t out_buf_size, size_t *output_size);

/**
 * @brief Notify the decoder that the input stream is finished.
 *
 * Notify the decoder that the input stream is finished. If the return value is
 * HSDR_FINISH_MORE, there is still more output, so call `heatshrink_decoder_poll` and repeat.
 *
 * @param hsd Pointer to the decoder.
 *
 * @return HSD_finish_res value indicating the result of the operation.
 */
HSD_finish_res heatshrink_decoder_finish(heatshrink_decoder *hsd);

#endif /* HEATSHRINK_DECODER_H */
